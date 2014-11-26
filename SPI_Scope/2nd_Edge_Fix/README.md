By: Aaron Wisner

All Images show the two bytes 00000001 01011101 being written

Top Trace: CLK
Bottom trace: MOSI


Before Changes:

1st_Edge_High_Idle: Shows the library working properly when set to write on 1st clock edge, with high clock idle

2nd_Edge_High_Idle: Shows the Libraries output when set to wrte on 2nd clock edge with high clock idle. As you can see, it doesnt write correct two bytes and doesn't respsect the command to idle clock high

2nd_Edge_Low_Idle: Shows the Libraries output when set to wrte on 2nd clock edge with low clock idle. As you can see, it doesnt write correct two bytes and doesn't respsect the command to idle clock low


After Changes:

2nd_Edge_High_Idle (After Bug Fixes): Now the library properly writes the correct two bytes and respects the command to idle clock high and low (low not shown)
