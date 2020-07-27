#ifndef LOGGEROUTPUT_H
#define LOGGEROUTPUT_H

namespace Freehuni
{
	class LoggerOutput
	{
	public:
		LoggerOutput()
		{}
		virtual ~LoggerOutput(){}
		virtual void Print(const char* fmt, ...) = 0;
	};


	class ConsoleOutput : public LoggerOutput
	{
	public:
		ConsoleOutput()
		{}
		void Print(const char* fmt, ...) override;

	};

	class ColorConsoleOutput : public ConsoleOutput
	{
	public:
		typedef enum
		{
			eBlack=30,
			eRed,
			eGreen,
			eYellow,
			eBlue,
			eMagenta,
			eCyan,
			eWhite
		} eForeColor;

		typedef enum
		{
			eBackBlack=40,
			eBackRed =41,
			eBackGreen=42,
			eBackYellow=43,
			eBackBlue=44,
			eBackMagenta=45,
			eBackCyan=46,
			eBackWhite=47
		} eBackColor;

		ColorConsoleOutput()
		{}
		void Print(const char* fmt, ...) override;
		void Print(eForeColor foreColor, eBackColor BackColor, const char* fmt, ...);

	};

	class UdpOutput : public LoggerOutput
	{
	public:
		UdpOutput(){}

	protected:
		void Print(const char* fmt, ...) override;
	};
}

#endif // LOGGEROUTPUT_H
