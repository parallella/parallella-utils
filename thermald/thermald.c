#include <stdio.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

#if TEST
#ifndef DEBUG
#define DEBUG 1
#endif
#define TEMP_DIR         "/tmp/thermald/"
#else
#define TEMP_DIR         "/sys/class/hwmon/hwmon0/"
#endif

#define TEMP_INPUT_PATH  (TEMP_DIR "temp1_input")

#define EPIPHANY_DEVICE "/dev/epiphany/mesh0"
const char *epiphany_device = EPIPHANY_DEVICE;

/* In Celsius */
#define DEFAULT_MIN_TEMP 0
#define DEFAULT_MAX_TEMP 70

/* Will shutdown at this temperature */
#define CRITICAL_MAX_TEMP 70

/* Allowed range for user specified THERMALD_{MIN,MAX}_TEMP environment
 * variables */
#define ENV_ALLOWED_MIN_TEMP DEFAULT_MIN_TEMP
#define ENV_ALLOWED_MAX_TEMP CRITICAL_MAX_TEMP


/* Both in seconds */
#define MAINLOOP_ITERATION_INTERVAL 1
#define MAINLOOP_WARN_INTERVAL 30

struct watchdog {
	int min_temp;  /* Min allowed temperature (in Celcius) */
	int max_temp;  /* Max allowed temperature (in Celcius) */
	int curr_temp; /* Current temperature     (in Celcius) */
};
#define DECLARE_WATCHDOG(Name) struct watchdog (Name) = \
	{ DEFAULT_MIN_TEMP, DEFAULT_MAX_TEMP, (DEFAULT_MAX_TEMP+1) }


/* Set by signal handler */
volatile sig_atomic_t exit_signaled = 0;


void signal_handler(int sig)
{
	(void) sig;
	fprintf(stderr, "Got %s\n", strsignal(sig));
	exit_signaled = 1;
}

int update_temp_sensor(struct watchdog *wd)
{
	FILE *fp;
	int rc, millicelsius;

	fp = fopen(TEMP_INPUT_PATH, "r");
	if (fp == NULL)
		return errno;

	/* Get temperature in millicelsius */
	rc = fscanf(fp, "%d", &millicelsius);
	if (rc != 1) {
		fclose(fp);
		return ENODATA;
	}

	/* Round to nearest */
	wd->curr_temp = (millicelsius + 500) / 1000;

	fclose(fp);

#if DEBUG
	printf("%s(): Current temp: %d\n", __func__, wd->curr_temp);
	fflush(stdout);
#endif
	return 0;
}

void print_warning(struct watchdog *wd, const char *limit)
{
	fprintf(stderr, "Disabling Epiphany chip. Temperature [%d C] is %s"
			" allowed range [[%d -- %d] C].\n",
			wd->curr_temp, limit, wd->min_temp,
			wd->max_temp);
}

void print_enable(struct watchdog *wd)
{
	fprintf(stderr, "Enabling Epiphany chip. Temperature [%d C] is within"
			" allowed range [[%d -- %d] C].\n",
			wd->curr_temp, wd->min_temp, wd->max_temp);
}

void print_shutdown(struct watchdog *wd)
{
	fprintf(stderr, "SHUTTING DOWN SYSTEM! Temperature [%d C] is above"
			" MAX CRITICAL TEMPERATURE [%d C].\n",
			wd->curr_temp, wd->max_temp);
}


int mainloop(struct watchdog *wd)
{
	int rc;
	bool shutdown;
	const char *limit;

	while (!exit_signaled) {
		rc = update_temp_sensor(wd);
		if (rc) {
			/* Try one more time before giving up */
			sleep(1);
			rc = update_temp_sensor(wd);
			if (rc) {
				perror("ERROR: Failed to update temperature sensor value\n");
				return rc;
			}
		}

		shutdown  = wd->curr_temp < wd->min_temp;
		shutdown |= wd->curr_temp > wd->max_temp;

		if (shutdown) {
			limit = wd->curr_temp <= wd->min_temp ?
				"below" : "above";
			print_warning(wd, limit);

			print_shutdown(wd);
			sync();
			system("shutdown -h now");
		}

		sleep(MAINLOOP_ITERATION_INTERVAL);
	}

	return 0;
}

void get_limits_from_env(struct watchdog *wd)
{
	int rc, tmp, env_min, env_max;
	char *str;

	env_min = wd->min_temp;
	env_max = wd->max_temp;

	str = getenv("THERMALD_MAX_TEMP");
	if (str) {
		rc = sscanf(str, "%d", &tmp);
		if (rc == 1) {
			env_max = tmp;
		} else {
			fprintf(stderr, "Ignoring malformed"
				       " THERMALD_MAX_TEMP\n");
		}
	}
	str = getenv("THERMALD_MIN_TEMP");
	if (str) {
		rc = sscanf(str, "%d", &tmp);
		if (rc == 1) {
			env_min = tmp;
		} else {
			fprintf(stderr, "Ignoring malformed"
				       " THERMALD_MIN_TEMP\n");
		}
	}

	/* Range check for insane values */
	if (env_max <= env_min) {
		fprintf(stderr, "Ignoring insane THERMALD_{MIN,MAX}_TEMP"
				" environment values.\n");
		return;
	}

	if (env_max <= ENV_ALLOWED_MAX_TEMP &&
		ENV_ALLOWED_MIN_TEMP < env_max)
		wd->max_temp = env_max;
	else {
		fprintf(stderr, "Ignoring THERMALD_MAX_TEMP value.\n");
	}

	if (ENV_ALLOWED_MIN_TEMP <= env_min &&
			env_min < wd->max_temp)
		wd->min_temp = env_min;
	else {
		fprintf(stderr, "Ignoring THERMALD_MIN_TEMP value.\n");
	}

}

int main(int argc, char **argv)
{
	int rc;
	DECLARE_WATCHDOG(wd);

	fprintf(stderr, "Parallella thermal watchdog daemon starting...\n");

	get_limits_from_env(&wd);

	fprintf(stderr, "Allowed temperature range [%d -- %d] C.\n",
			wd.min_temp, wd.max_temp);

	if (argc > 1)
		epiphany_device = argv[1];

	fprintf(stderr, "Using %s\n", epiphany_device);

	/* Ensure we can access the XADC temperature sensor (via hwmon) */
	rc = update_temp_sensor(&wd);
	if (rc) {
		perror("ERROR: Temperature sensor sysfs entries not present");
		fprintf(stderr, "Make sure to compile your kernel with \"CONFIG_IIO=y\", \"CONFIG_XILINX_XADC=y\", \"CONFIG_HWMON=y\", and \"CONFIG_SENSORS_IIO_HWMON=y\".\n");

		return rc;
	}

	/* Set up SIGTERM handler */
	signal (SIGTERM, signal_handler);
	signal (SIGINT, signal_handler);

	fprintf(stderr, "Entering mainloop.\n");
	rc = mainloop(&wd);
	if (rc) {
		fprintf(stderr, "ERROR: mainloop failed\n");
	} else {
		fprintf(stderr, "Exiting normally\n");
	}

	return rc;
}
