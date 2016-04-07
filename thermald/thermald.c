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
#include <misc/epiphany.h>

#if TEST
#ifndef DEBUG
#define DEBUG 1
#endif
#define TEMP_DIR         "/tmp/thermald/"
#else
#define TEMP_DIR         "/sys/bus/iio/devices/iio:device0/"
#endif

#define TEMP_RAW_PATH    (TEMP_DIR "in_temp0_raw")
#define TEMP_OFFSET_PATH (TEMP_DIR "in_temp0_offset")
#define TEMP_SCALE_PATH  (TEMP_DIR "in_temp0_scale")

#define EPIPHANY_DEVICE "/dev/epiphany/mesh0"
const char *epiphany_device = EPIPHANY_DEVICE;

/* In Celsius */
#define DEFAULT_MIN_TEMP 0
#define DEFAULT_MAX_TEMP 70

/* Will shutdown at this temperature */
#define CRITICAL_MAX_TEMP 85

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

int disallow_mesh_access()
{
	int fd, rc;

#if DEBUG
	printf(__func__ "(): Was called\n");
#endif
	fd = open(epiphany_device, O_RDWR);

	rc = ioctl(fd, E_IOCTL_THERMAL_DISALLOW);

	(void)(rc);

	close(fd);

	return 0;
}

int allow_mesh_access()
{
	int fd, rc;

#if DEBUG
	printf(__func__ "(): Was called\n");
#endif
	fd = open(epiphany_device, O_RDWR);

	rc = ioctl(fd, E_IOCTL_THERMAL_ALLOW);

	(void)(rc);

	close(fd);

	return 0;
}

int update_temp_sensor(struct watchdog *wd)
{
	FILE *fp;
	int rc, raw, offset;
	float scale;

	/* Get raw temperature */
	fp = fopen(TEMP_RAW_PATH, "r");
	if (fp == NULL)
		return errno;

	rc = fscanf(fp, "%d", &raw);
	if (rc != 1)
		return ENODATA;

	fclose(fp);

	/* Get offset */
	fp = fopen(TEMP_OFFSET_PATH, "r");
	if (fp == NULL)
		return errno;

	rc = fscanf(fp, "%d", &offset);
	if (rc != 1)
		return ENODATA;

	fclose(fp);

	/* Get scale */
	fp = fopen(TEMP_SCALE_PATH, "r");
	if (fp == NULL)
		return errno;

	rc = fscanf(fp, "%f", &scale);
	if (rc != 1)
		return ENODATA;

	fclose(fp);

	/* Calculate temperature */
	wd->curr_temp = (int)
		roundf((scale / 1000.0) * (((float) raw) + ((float) offset)));


	return 0;
}

void print_warning(struct watchdog *wd, char *limit)
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
			wd->curr_temp, CRITICAL_MAX_TEMP);
}


int mainloop(struct watchdog *wd)
{
	int rc, last_warning;
	bool should_warn, enable_next = true, disallowed = false;

	last_warning = -1;

	while (!exit_signaled) {
		rc = update_temp_sensor(wd);
		if (rc) {
			/* Try one more time before giving up */
			sleep(1);
			rc = update_temp_sensor(wd);
			if (rc) {
				perror("ERROR: Failed to update temperature"
						" sensor value\n");
				return rc;
			}
		}

		/* Limit log spamming */
		should_warn = (last_warning < 0 ||
				last_warning >= MAINLOOP_WARN_INTERVAL) ?
			true : false;

		if (wd->min_temp > wd->curr_temp) {
			if (should_warn) {
				print_warning(wd, "below");
				last_warning = 0;
			} else {
				last_warning += MAINLOOP_ITERATION_INTERVAL;
			}
			disallowed = true;
			disallow_mesh_access();
		} else if (wd->curr_temp > wd->max_temp) {
			if (should_warn) {
				print_warning(wd, "above");
				last_warning = 0;
			} else {
				last_warning += MAINLOOP_ITERATION_INTERVAL;
			}
			disallowed = true;
			disallow_mesh_access();

			if (wd->curr_temp > CRITICAL_MAX_TEMP) {
				print_shutdown(wd);
				sync();
				system("shutdown -h now");
			}
		} else {
			last_warning = -1;
			if (disallowed) {
				/* Give it some time to cool down */
				sleep(MAINLOOP_WARN_INTERVAL);
				disallowed = false;
				enable_next = true;
				continue;
			} else {
				if (enable_next) {
					print_enable(wd);
					enable_next = false;
				}
				allow_mesh_access();
			}
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

#if DEBUG
	e_set_host_verbosity(H_D1);
#endif

	/* Check that we can enable/disable access to the mesh */
	rc = disallow_mesh_access();
	if (rc) {
		fprintf(stderr, "ERROR: Disabling Epiphany chip failed.\n");
		return rc;
	}
	rc = allow_mesh_access();
	if (rc) {
		fprintf(stderr, "ERROR: Enabling Epiphany chip failed.\n");
		goto out;
	}

	/* Ensure we can access the XADC temperature sensor */
	rc = update_temp_sensor(&wd);
	if (rc) {
		perror("ERROR: Temperature sensor sysfs entries not present");
		fprintf(stderr, "Make sure to compile your kernel with"
			" \"CONFIG_IIO=y\" and \"CONFIG_XILINX_XADC=y\".\n");

		goto out;
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

out:
	allow_mesh_access();
	return rc;
}
