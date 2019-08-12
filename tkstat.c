#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <X11/Xlib.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>

static Display *dpy;

#define BUFSIZE 256
#define BAT_STATUS_FILE "/sys/class/power_supply/BAT0/status"
#define BAT_ENERGY_NOW_FILE "/sys/class/power_supply/BAT0/energy_now"
#define BAT_ENERGY_FULL_FILE "/sys/class/power_supply/BAT0/energy_full"
#define BAT_POWER_NOW_FILE "/sys/class/power_supply/BAT0/power_now"
#define BAT_CAPACITY_FILE "/sys/class/power_supply/BAT0/capacity"
#define TEMPERATURE_FILE "/sys/class/thermal/thermal_zone0/temp"
#define FAN_FILE "/proc/acpi/ibm/fan"

float readFloat(const char *file)
{
	float ret = 0;
	FILE *fp;
	if (!(fp = fopen(file, "r"))) {
		perror("fopen");
		return 0;
	}
	if (fscanf(fp, "%f", &ret) != 1) {
		fclose(fp);
		return 0;
	}
	fclose(fp);
	return ret;
}

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
	if(!(fp = fopen(BAT_STATUS_FILE, "r"))) {
		perror("fopen");
		return 0;
	}
	state = fgetc(fp);
	fclose(fp);
	return tolower(state);
}

int getBatPercent() 
{
	return readFloat(BAT_CAPACITY_FILE);
}

int getBatRemainingMins()
{
	float energy_now = readFloat(BAT_ENERGY_NOW_FILE);
	float power_now = readFloat(BAT_POWER_NOW_FILE);
	if (power_now == 0) {
		return 0;
	}
	return 60 * energy_now / power_now;
}

int getBatRemainigChargingMins()
{
	float energy_full = readFloat(BAT_ENERGY_FULL_FILE);
	float energy_now = readFloat(BAT_ENERGY_NOW_FILE);
	float power_now = readFloat(BAT_POWER_NOW_FILE);
	if (power_now == 0) {
		return 0;
	}
	return 60 * (energy_full - energy_now) / power_now;
}

int getTemp() 
{
	return readFloat(TEMPERATURE_FILE) / 1000;
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

void sigHandler(int signo) {
	if(signo == SIGTERM || signo == SIGINT) {
		printf("Receiving term\n");
		setStatus("Powering off              ");
		exit(0);
	} else {
		printf("got signo: %d\n", signo);
	}
}

int main() 
{
	char state;
	char buf[BUFSIZE];
	int remaining_mins, remaining_percent, len;

	// setup signals //
	if(((signal(SIGTERM, sigHandler) == SIG_ERR)) || (signal(SIGINT, sigHandler) == SIG_ERR)) {
		fprintf(stderr, "Cannot setup signals\n");
		return 1;
	}

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
		remaining_percent = getBatPercent();
		// state is battery charging //
		if (state == 'c') {
			remaining_mins = getBatRemainigChargingMins();
			len += snprintf(buf+len, sizeof(buf)-len, "AC: %2d%% %02d:%02d | ",
				remaining_percent, remaining_mins / 60, remaining_mins % 60);
		}
		// state is battery discharging //
		else if (state == 'd') {
			remaining_mins = getBatRemainingMins();
			if(remaining_mins == 0 || remaining_percent == 0) {
				continue;
			}
			len += snprintf(buf+len, sizeof(buf)-len, "BAT: %2d%% %02d:%02d | ",
				remaining_percent, remaining_mins / 60, remaining_mins % 60);
		} else {
			len += snprintf(buf+len, sizeof(buf)-len, "AC: %2d%% | ", remaining_percent);
		}

		len += getDateTime(buf + len, sizeof(buf) - len);
		// display //
		setStatus(buf);
	}
	
	XCloseDisplay(dpy);
	return 0;
}
