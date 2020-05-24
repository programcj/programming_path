#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>
#include <sys/resource.h>
#include <termios.h>
#include <ustat.h>
#include <sys/utsname.h>
#include <pwd.h>
#include <utmpx.h>
#include <sys/stat.h>
#include <shadow.h>
#include <crypt.h>

int linuxos_login(const char *username, const char *password) {
	struct passwd *pw;
	struct passwd pwdstruct;
	char pwdbuf[256];
	struct spwd *spasswd = NULL;

	char *passwd_crypt;

	getpwnam_r(username, &pwdstruct, pwdbuf, sizeof(pwdbuf), &pw);
	if (!pw) {
		return -1;
	}
	//				printf("my name = [%s]\n", pw->pw_name);
	//				printf("my passwd = [%s]\n", pw->pw_passwd);
	//				printf("my uid = [%d]\n", pw->pw_uid);
	//				printf("my gid = [%d]\n", pw->pw_gid);
	//				printf("my gecos = [%s]\n", pw->pw_gecos);
	//				printf("my dir = [%s]\n", pw->pw_dir);
	//				printf("my shell = [%s]\n", pw->pw_shell);
	if (pw->pw_passwd[0] == '!' || pw->pw_passwd[0] == '*')
		return -1; // *帐号被停用;
	passwd_crypt = pw->pw_passwd;
	spasswd = getspnam(pw->pw_name);

	if (spasswd && spasswd->sp_pwdp) {
		passwd_crypt = spasswd->sp_pwdp;
	}
	if (!passwd_crypt) {
		passwd_crypt = "!!";
	}

	//1> $1$开头，以$结尾，那么这表示让crypt用MD5的方式加密，加密后出来的密文格式就是 $1$...$<密文正文>
	char *testcrypt = NULL;
	testcrypt = crypt(password, passwd_crypt);

	if (testcrypt && strcmp(testcrypt, passwd_crypt) == 0) {
		return 0;
	}
	return -1;
}

int change_user_shell(const char *username) {
	struct passwd *pw;
	struct passwd pwdstruct;
	char pwdbuf[256];
	getpwnam_r(username, &pwdstruct, pwdbuf, sizeof(pwdbuf), &pw);
	if (!pw) {
		return -1;
	}

	fchown(0, pw->pw_uid, pw->pw_gid); //STDIN_FILENO
	fchmod(0, 0600);

	if (pw->pw_uid == 0)
		syslog(LOG_INFO, "root login%s", username);
	{
		const char *args[2] = { NULL, NULL };
		char *slash = strrchr(pw->pw_shell, '/');
		char arg1[100];

		if (!slash || (slash == pw->pw_shell && !slash[1]))
			slash = pw->pw_shell;
		else
			slash += 1;
		sprintf(arg1, "-%s", slash);
		args[0] = arg1;

		struct utmpx utent;
		struct utmpx *utp;

		//touch(_PATH_UTMPX);
		//utmpxname(_PATH_UTMPX);
		execv(pw->pw_shell, args);
		printf("can't execute '%s'", pw->pw_shell);
	}
	return 0;
}


int main(int argc, char **argv){
	char username[50];
	char password[50];
	int ret;
	
	strcpy(username, "root");
	strcpy(password, "123456");

	ret = linuxos_login(username, password);
	if(ret!=0)
		return -1;

	signal(SIGINT, SIG_DFL);
	change_user_shell(username);

	return 0;
}
