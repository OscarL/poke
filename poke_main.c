/*
 * Copyright 2005-2022 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oscar Lesta, oscar.lesta@gmail.com
 */


#include "poke_commands.h"
#include "poke_io.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <FindDirectory.h>

////////////////////////////////////////////////////////////////////////////////

void command_about(int argc, uint32 argv[]);
void command_beep(int argc, uint32 argv[]);
void command_clear(int argc, uint32 argv[]);
void command_help(int argc, uint32 argv[]);
void command_nummode(int argc, uint32 argv[]);
void command_quit(int argc, uint32 argv[]);

status_t process_line(char line[]);
int index_for_command(const char name[]);
char* trimstring(char str[]);


#ifdef USE_EDITLINE
	#include <editline/readline.h>
	// Interface to lib{edit|read}line.so completion
	char* command_generator(const char text[], int);
	char** command_completion(const char text[], int start, int end);
#endif


////////////////////////////////////////////////////////////////////////////////

typedef struct {
	const char* name;
	void (*poke_cmd)(int argc, uint32 argv[]);
	const char* args;
	const char* help;
} command;


// I find original poke's naming to be confusing.
// I choose to go with a more "traditional" convention, ie:
// 8-bits = Byte, 16-bits = Word, 32-bits = Long.

command commands[] = {
	{  "db",		command_db,			"address",					"Display a byte in virtual space" },
	{  "dw",		command_dw,			"address",					"Display a word (16 bits)" },
	{  "dl",		command_dl,			"address",					"Display a long (32 bits)" },
	{  "dm",		command_dm,			"address [count]",			"Display 'count' memory bytes" },

	{  "sb",		command_sb,			"address value",			"Set a byte" },
	{  "sw",		command_sw,			"address value",			"Set a word (16 bits)" },
	{  "sl",		command_sl,			"address value",			"Set a long (32 bits)" },

	{ "dpb",		command_dpb,		"address",					"Display a byte in physical space" },
	{ "dpw",		command_dpw,		"address",					"Display a word (16 bits) in phys space" },
	{ "dpl",		command_dpl,		"address",					"Display a long (32 bits) in phys space" },
	{ "dpm",		command_dpm,		"address [count]",			"Display 'count' bytes from phys memory" },

	{ "spb",		command_spb,		"address value",			"Set a byte in physical space" },
	{ "spw",		command_spw,		"address value",			"Set a word (16 bits) in physical space" },
	{ "spl",		command_spl,		"address value",			"Set a long (32 bits) in physical space" },

	{ "dumpvm",		command_dumpvm,		"address count n",			"Dump 'count' pages of virt mem to file n" },
	{ "dumppm",		command_dumppm,		"address count n",			"Dump 'count' pages of phys mem to file n" },

	{ "inb",		command_inb,		"port [last_port]",			"Read I/O byte(s) from port(s)" },
	{ "inw",		command_inw,		"port",						"Read an I/O word" },
	{ "inl",		command_inl,		"port",						"Read an I/O long" },

	{ "outb",		command_outb,		"port value",				"Set an I/O byte" },
	{ "outw",		command_outw,		"port value",				"Set an I/O word" },
	{ "outl",		command_outl,		"port value",				"Set an I/O long" },

	{ "idxinb",		command_idxinb,		"port index [last-idx]",	"Display VGA-style indexed reg(s)" },
	{ "idxoutb",	command_idxoutb,	"port index value",			"Write a VGA-style indexed reg" },

	{ "pci",		command_pci,		"[bus dev fun]",			"Display PCI device info" },
	{ "pcilist",	command_pcilist,	"",							"List PCI devices" },

	{ "cfinb",		command_cfinb,		"bus dev fun off [last-off]",	"Read PCI config byte(s)" },
	{ "cfinw",		command_cfinw,		"bus dev fun off [last-off]",	"Read PCI config word(s)" },
	{ "cfinl",		command_cfinl,		"bus dev fun off [last-off]",	"Read PCI config long(s)" },

	{ "cfoutb",		command_cfoutb,		"bus dev fun off val",		"Write a PCI config byte" },
	{ "cfoutw",		command_cfoutw,		"bus dev fun off val",		"Write a PCI config word" },
	{ "cfoutl",		command_cfoutl,		"bus dev fun off val",		"Write a PCI config long" },

#ifdef DONT_USE_LINE_EDITING
// ToDo:
//	{ "r",			command_repeat,		"",							"Repeat last command" },
#endif

	{ "?",			command_help,		"[command]",				"Show help (for [command])" },
	{ "help",		command_help,		"[command]",				"Show help (for [command])" },
	{ "num",		command_nummode,	"",							"Toggle numeric args mode (Hex/Dec)" },
	{ "clear",		command_clear,		"",							"Clear the console" },
#if defined(__i386__) || defined(__x86_64__)
	{ "beep",		command_beep,		"[freq] [duration_ms]",		"beep beep!" },
#endif
	{ "quit",		command_quit,		"",							"Quit poking around" },
	{ "exit",		command_quit,		"",							"Quit poking around" },
	{ "about",		command_about,		"",							"About this program" },
};

static const int kCommandsCount = (sizeof(commands) / sizeof(command));

static int done = 0;

// all arguments expected to be in:
enum {
	kHexMode = 0,	// Hex, ie: 32 == 0x32
	kDecMode		// Dec, ie: 32 != 0x32, 32 == 0x20
};

static int gNumMode = kDecMode;

////////////////////////////////////////////////////////////////////////////////

#if defined(__BEOS__) || defined(__HAIKU__)
	#define _kNormal_	"\e[m"
	#define _kBold_		"\e[0;1m"
	#define _kRed_		"\e[0;1;31m"
	#define _kGreen_	"\e[0;1;32m"
	#define _kYellow_	"\e[0;1;33m"

	#define	HAIKU_COLOR_STRING _kGreen_"H"_kRed_"a"_kBold_"ik"_kYellow_"u"_kNormal_

	#define	INTRO_STRING "\nWelcome to "HAIKU_COLOR_STRING"'s poke shell! (type 'help' if you need it)\n"
#else
	#define	INTRO_STRING "\nWelcome to the poke shell! (type 'help' if you need it)\n"
#endif

////////////////////////////////////////////////////////////////////////////////
// Entry point

int main(int argc, char* argv[])
{
#ifdef USE_EDITLINE
	char history_file[B_FILE_NAME_LENGTH] = {'\0'};
	char* line;
#else
	char line[255];
#endif
	status_t status;
	char* s;

	// If not a tty spawn a Terminal window, unless we're called
	// with arguments (like "poke -"), read from stdin in that case.

	// review this, at start I get "poke: poke: something" when I hit cursor up.
	if (!isatty(STDIN_FILENO) && (argc == 1)) {
		char command[256 + 10];
		sprintf(command, "Terminal -t \"Poke Shell\" \"%s\"", argv[0]);
		system(command);
		return B_OK;
	}

	system("sync"); // Better safe than sorry.

	status = open_poke_driver();
	if (status < B_OK) {
		printf("Couldn't open the poke driver, reason: %s\n", strerror(status));
		return B_ERROR;
	}

	printf(INTRO_STRING);
	printf("(Numeric arguments will be interpreted as %s)\n\n",
			(gNumMode == kDecMode) ? "Decimal" : "Hexadecimal");

#ifdef USE_EDITLINE
	status = find_directory(B_USER_SETTINGS_DIRECTORY, 0, false, history_file, sizeof(history_file));
	if (status == B_OK) {
		strcat(history_file, "/poke_history");
	}

	rl_readline_name = "poke"; // allow custom keybindings in ~/.inputrc
	rl_attempted_completion_function = command_completion;

	// Disable completion for now, as it currently crashes :-(
	rl_inhibit_completion = true;

	if (strlen(history_file))
		read_history(history_file);
#endif

	while (done == 0) {
#ifdef USE_EDITLINE
		line = readline("poke: ");
		if (line == NULL)
			continue;
#else
		fflush(stdout);
		printf("poke: ");
		fflush(stdout);

		if (fgets(line, 255, stdin) == NULL)
			continue;
#endif

		s = trimstring(line);
		if (*s) {
#ifdef USE_EDITLINE
			add_history(s);
#endif
			process_line(s);
		}
#ifdef USE_EDITLINE
		free(line);
#endif
	}

#ifdef USE_EDITLINE
	if (strlen(history_file))
		write_history(history_file);
#endif

	close_poke_driver();

	return B_OK;
}


#ifdef USE_EDITLINE

char** command_completion(const char text[], int start, int end)
{
	char** matches = NULL;

	// If using libreadline, don't default to complete-filenames.
//	rl_attempted_completion_over = 1;

	// If this word is at the start of the line, then it is a command to
	// complete.
	if (start == 0 || start <= 5) {
		matches = completion_matches((char*)text, command_generator);
//		matches = rl_completion_matches(text, command_generator);
	}
	return matches;
}


char* command_generator(const char text[], int state)
{
	static int i, len;

	if (!state) {
		i = -1;
		len = strlen(text);
	}

	// Return the next name which partially matches from the command list.
	while (i < kCommandsCount) {
		i++;
		if ((commands[i].name) && (strncmp(commands[i].name, text, len) == 0)) {
			return strdup(commands[i].name);
		}
    }

	return NULL;
}

#endif	// #ifdef USE_EDITLINE


////////////////////////////////////////////////////////////////////////////////
//	#pragma mark -

status_t process_line(char line[])
{
	uint32 argv[5];	// to hold the command arguments.
	int cmd_index;
	uint8 i, argc = 0;
	char* command;
	char* argument;

	// Get the requested command:
	command = strtok(line, " "); // I love "The C Library Reference Guide"
	if (command == NULL)
		return B_ERROR;

	cmd_index = index_for_command(command);
	if (cmd_index < 0) {
		fprintf(stderr, "Sorry, Me no comprende.\n");
		return B_ERROR;
	}

	// The help/? commands are special cases:
	if (strncmp(command, "help", 4) && strncmp(command, "?", 1)) {
		for (i = 0; i < 5; i++) {
			argument = strtok(NULL, " ");
			if (argument == NULL)
				break;

			switch (gNumMode) {
				case kHexMode:
					argv[i] = strtoul(argument, NULL, 16);
					break;
				case kDecMode:
				default:
					argv[i] = strtoul(argument, NULL, 0);
					break;
			}

			argc++;

			if (errno == ERANGE) {
				printf("Argument #%d is out of range\n", i + 1);
				break;
			}
		}
	} else {
		argument = strtok(NULL, " ");
		if (argument != NULL) {
			argv[0] = index_for_command(argument);
			argc = 1;
		}
	}

	commands[cmd_index].poke_cmd(argc, argv);

	return B_OK;
}


int index_for_command(const char name[])
{
	int i;
	for (i = 0; i < kCommandsCount; i++) {
		if (strcmp(name, commands[i].name) == 0)
			return i;
	}

	return B_ERROR;
}


char* trimstring(char str[])
{
	char *start, *end;

	start = str;
	while (isspace(*start)) // skip leading whites...
		start++;

	if (*start == '\0') // are we at string's end yet?
		return start;

	end = start + strlen(start) - 1;
	while ((end > start) && isspace(*end)) // skip trailing whites...
		end--;

	*++end = '\0';

	return start;
}


////////////////////////////////////////////////////////////////////////////////
//	#pragma mark - Misc commands

void command_help(int argc, uint32 argv[])
{
	int i;
	int printed = 0;

	// "help"
	if (argc == 0) {
		printf("%-10s%-28s%s.\n", "Command", "Arguments", "Meaning");
		printf("===============================================================================\n");
		for (i = 0; i < kCommandsCount; i++)
			printf("%-10s%-28s%s.\n", commands[i].name, commands[i].args,
					commands[i].help);
		return;
	}

	// "help command"
	if (argc == 1 && ((signed) argv[0]) > B_ERROR) {
		printf("%-10s%-28s%s.\n", commands[argv[0]].name,
				commands[argv[0]].args, commands[argv[0]].help);
		return;
	}

	printf("No such command. Possible ones are:\n");
	printed = 0;
	for (i = 0; i < kCommandsCount; i++) {
		if (printed == 8) {
			printed = 0;
			printf("\n");
		}
		printf("%s\t", commands[i].name);
		printed++;
	}

	if (printed)
		printf("\n");
}


// No more poking around? Bummer!
void command_quit(int argc, uint32 argv[])
{
	done = 1;
}


void command_clear(int argc, uint32 argv[])
{
#if defined(__MSDOS__) || defined(__WIN32__)
	system("cls");
#else
	// how do we clear the screen buffer? (^L)
	system("tput clear");
#endif
}


void command_nummode(int argc, uint32 argv[])
{
	// toogle the mode and report.
	if (argc == 0) {
		(gNumMode == kDecMode) ? (gNumMode = kHexMode) : (gNumMode = kDecMode);
		printf("Will expect num args as: %s\n",
				(gNumMode == kDecMode) ? "Decimal" : "Hexadecimal");
	}
}


#if defined(__i386__) || defined(__x86_64__)

void command_beep(int argc, uint32 argv[])
{
	switch (argc) {
		case 0:
			pc_speaker_beep(1000, 250000);
			break;
		case 1:
			pc_speaker_beep(argv[0], 250000);
			break;
		case 2:
			pc_speaker_beep(argv[0], (argv[1] <= 2000) ? (argv[1] * 1000): 2000000);
			break;
		default:
			printf("Wrong number of arguments\n");
			break;
	}
}

#endif


void command_about(int argc, uint32 argv[])
{
	if (argc)
		printf("About whom?\n");
	else {
		printf("Copyright 2005, Haiku Inc.\n");
		printf("This program was brought to you by Oscar Lesta\nBig Deal!\n");
	}
}
