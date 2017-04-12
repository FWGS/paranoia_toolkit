

#ifndef LOG_H
#define LOG_H



void SetupLogging ();
void Log (const char * fmt, ...);
void ConLog (const char * fmt, ...);


#define ONCE( somecode )	\
do {						\
	static int called = 0;	\
	if (!called)			\
	{						\
		somecode			\
		called = 1;			\
	}						\
} while (0)



#endif