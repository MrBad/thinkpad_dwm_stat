#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <time.h>
#include <unistd.h>


static Display *dpy;

#define BUFSIZE 256
#define BAT_ST_FILE "/sys/devices/platform/smapi/BAT0/state"
#define BAT_RP_FILE "/sys/devices/platform/smapi/BAT0/remaining_percent"
#define BAT_RM_FILE "/sys/devices/platform/smapi/BAT0/remaining_running_time_now"
#define BAT_RCM_FILE "/sys/devices/platform/smapi/BAT0/remaining_charging_time"

#define TEMP_FILE	"/sys/class/thermal/thermal_zone1/temp"
#define FAN_FILE	"/proc/acpi/ibm/fan"

void setStatus(char *str) 
{
	XStoreName(dpy, DefaultRootWindow(dpy), str);
	XSync(dpy, False);
}


// return len of the writed date in buf
int getDateTime(char *buf, int buf_size) 
{
	time_t ts;
	struct tm *tm;
	int len;

	ts = time(NULL);
	tm = localtime(&ts);
	len = strftime(buf, buf_size-1, "%a, %b %d %H:%M:%S", tm);
	if(!len) {
		fprintf(stderr, "strftime return 0\n");
		return 0;
	}
	return len;
}

char getState() 
{
	FILE *fp;
	char state;
	if(!(fp = fopen(BAT_ST_FILE, "r"))) {
		perror("fopen");
		return 0;
	}
	state = fgetc(fp);
	fclose(fp);
	return state;
}


int getBatRemaining(const char *file) 
{
	FILE *fp;
	int remaining;
	if(!(fp = fopen(file, "r"))) {
		perror("fopen");
		return 0;
	}
	if(fscanf(fp, "%d", &remaining) != 1) {
		fclose(fp);
		return 0;
	}
	fclose(fp);
	return remaining;
}

int getTemp() 
{
	FILE *fp;
	int temp;
	if(!(fp = fopen(TEMP_FILE, "r"))) {
		perror("fopen");
		return 0;
	}
	if(fscanf(fp, "%2d", &temp) != 1) {
		perror("fscanf");
		fclose(fp);
		return 0;
	}
	fclose(fp);
	return temp;
}

int getFanRPM()
{
	FILE *fp;
	char line[128];
	int rpm = 0;
	if(!(fp = fopen(FAN_FILE, "r"))) {
		perror("fopen");
		return 0;
	}

	while(!feof(fp)) {
		fgets(line, sizeof(line), fp);
		if(sscanf(line, "speed: %d\n", &rpm) == 1) {
			fclose(fp);
			return rpm;
		}
	}
	fclose(fp);
	return rpm;	
}

int main() 
{
	char state;
	char buf[BUFSIZE];
	int remaining_mins, remaining_percent, len;

	if(!(dpy = XOpenDisplay(NULL))) {
		fprintf(stderr, "Cannot open display\n");
		return 1;
	}
	
	for(;;) {
		sleep(1);	// sleep first, because we have some continue next and i don't 
					// want to read too fast
		len = 0;
		// fan, temperature //
		len += snprintf(buf, sizeof(buf) - len, "%dC %d RPM | ", 
				getTemp(), getFanRPM());
		
		// power //
		state = getState();
		remaining_percent = getBatRemaining(BAT_RP_FILE);
		// state is on battery discharging //
		if(state == 'd') {
			remaining_mins = getBatRemaining(BAT_RM_FILE);
			if(remaining_mins == 0 || remaining_percent == 0) {
				continue;
			}
			len += snprintf(buf+len, sizeof(buf)-len, "BAT: %2d%% %02d:%02d | ",
				remaining_percent, remaining_mins / 60, remaining_mins % 60);
		}
		// idle - show battery percent //
		else if(state == 'i') {
			len += snprintf(buf+len, sizeof(buf)-len, "AC: %2d%% | ", 
					remaining_percent);
		}
		// charging, show time left until full //
		else {
			remaining_mins = getBatRemaining(BAT_RCM_FILE);
			len += snprintf(buf+len, sizeof(buf)-len, "AC: %2d%% %02d:%02d | ",
				remaining_percent, remaining_mins / 60, remaining_mins % 60);
		}

		len += getDateTime(buf + len, sizeof(buf) - len);
		// display //
		setStatus(buf);
	}
	
	XCloseDisplay(dpy);
	return 0;
}
