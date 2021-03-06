#include <mintbind.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <malloc.h>
#include <string.h>

#include "vdi_login.h"
#include "environment.h"
#include "signals.h"
#include "verify.h"
#include "logon.h"
#include "limits.h"
#include "tiny_aes.h"
#include "list.h"
#include "debug.h"


const char kSpace  = ' ';
const char kTab    = '\t';
const char kCommas = '"';


char username[__LEN_USERNAME__ + 1] = "";
char password[__LEN_PASSWORD__ + 1] = "";

extern int EventLoop();

void *BuildLoginDialog(void *menuPtr);
void  HandleLoginDialog(void *dialogPtr, void *menuPtr);

void  BuildWelcomeString(char *string);
void *BuildMenu();

static char	*CleanLine(char *buffer);
sList		*ReadConfig();

short		vloginConfCorrupted = 0;


sList *list = NULL;


int main()
{
	void *dialogPtr;
	void *menuPtr;

 	#ifdef DEBUG
	 InitDebug();
	#endif
	 DEBUG("Starting init: ");
	InitTinyAES();
	 DEBUG("Ok\n");
	 DEBUG("Creating menu\n");
	menuPtr = BuildMenu();

	 DEBUG("Creating login dialog\n");
	dialogPtr = BuildLoginDialog(menuPtr);
	HandleLoginDialog(dialogPtr, menuPtr);

	 DEBUG("Exiting TAES, something wrong happend!");
	ExitTinyAES();

	#ifdef DEBUG
	 ExitDebug();
	#endif

	return 0;
}

void *BuildMenu()
{
	sElement *element;
	sCommand *commands;
	void *menuPtr = CreateMenu();
	int i;
	
	 DEBUG("Reading config: ");
	list = ReadConfig();
	 DEBUG("Ok\n");
	
	if (!list)
	{
		char menuItem[] = "/bin/sh";

		vloginConfCorrupted = 1;

		 DEBUG("Creating default menu: ");
		list = CreateList();

		commands = malloc(sizeof(sCommand));
				
		commands->menuItem = malloc(sizeof(menuItem));
		strcpy(commands->menuItem, menuItem);

		commands->command = malloc(sizeof(menuItem));
		strcpy(commands->command, menuItem);

		commands->childargv = malloc(sizeof(char *) * 2);
		commands->childargv[0] = menuItem;
		commands->childargv[1] = NULL;

		// set up defaults
		PushBack(list, commands);
 		 DEBUG("Ok\n");

		 DEBUG("Invalid vlogin.conf dialog: ");

		// if there is no valid command in vlogin.conf
		AlertDialog("Alert", "File '/etc/vlogin.conf'\nis missing or invalid!\n \nusing '/bin/sh'", "Ops", 0);
		 DEBUG("Ok\n");
	}

	 DEBUG("Adding menu items:\n");
	element = list->first;
	shell = commands = element->data;

	for(i = 0; i < list->itemCount; i++)
	{
		int index;

		 DEBUG(" item: %s\n command: %s\n", commands->menuItem, commands->command);
		AttachMenuItem(menuPtr, (void *)commands->menuItem);

		for (index = 0; commands->childargv[index]; index ++)
			DEBUG("  Argument[%d]: %s\n", index, commands->childargv[index]);

		element = (void *)element->next;
		commands = element != 0 ? element->data : 0;
	}
	 DEBUG("Done\n");

	return menuPtr;
}

void *BuildLoginDialog(void *menuPtr)
{
	void *dialogPtr;
	char string[30] 	= WELCOME;
	
	sRect window		= {vdiInfo[0] / 2 - 160, vdiInfo[1] / 2 - 86, vdiInfo[0] / 2 + 160, vdiInfo[1] / 2 + 86};
	sRect button0		= {16, 136, 92, 158};
	sRect button1		= {256, 137, 276, 157};
	sRect button2		= {284, 137, 304, 157};
	sRect box			= {16, 60, 304, 125};
	sRect editField0	= {112, 69, 296, 89};
	sRect editField1	= {112, 95, 296, 115};
	sRect string0		= {16, 46, 0, 0};
	sRect string1		= {24, 84, 0, 0};
	sRect string2		= {24, 110, 0, 0};
	sRect comboBox		= {98, 137, 244, 157};

	 DEBUG("Creating dialog: ");
	dialogPtr = CreateDialog(window, TITLE);
	 DEBUG("Ok\n");

	 DEBUG("BuildWelcomeString\n");
	BuildWelcomeString(string);
	 DEBUG("Done\n");
	 DEBUG("Attaching dialog items: ");
	AttachBox(dialogPtr, box, ""); /* NULL isn't handled correctly */
	AttachString(dialogPtr, string0, string);
	AttachString(dialogPtr, string1, LOGIN);
	AttachString(dialogPtr, string2, PASSWD);
	AttachEditField(dialogPtr, editField0, FIELD_NORMAL, __LEN_USERNAME__, username);
	AttachEditField(dialogPtr, editField1, FIELD_MASKED, __LEN_PASSWORD__, password);
	AttachButton(dialogPtr, button0, BUTTON_CENTER + BUTTON_DEFAULT, "Login to");
	AttachButton(dialogPtr, button1, BUTTON_CENTER, "R");
	AttachButton(dialogPtr, button2, BUTTON_CENTER, "S");
	AttachComboBox(dialogPtr, comboBox, !vloginConfCorrupted ? COMBO_BOX_NORMAL : COMBO_BOX_DISABLED, menuPtr);
	 DEBUG("Ok\n");

	 DEBUG("DrawDialog: ");
	DrawDialog(dialogPtr);
	 DEBUG("Ok\n");

	return dialogPtr;
}

void HandleLoginDialog(void *dialogPtr, void *menuPtr)
{
	sElement *element;

	struct passwd *user = NULL;
	char *rmthost = NULL;
	int i;

	if (!list) return;

	 DEBUG("StartingEventLoop\n");

	switch(EventLoop())
	{
		case 8:
			element = list->first;

			// get selected menu item id
			for(i = 0; i < GetMenuSelect(menuPtr); i++)
				element = (void *)element->next;

			shell = element != 0 ? element->data : shell;

			 DEBUG("Command: %s\n", shell->command);
			 DEBUG("ComboBox value: %d\n", GetMenuSelect(menuPtr));

			InstallHandlers();

			 DEBUG("Verify\n");

			if ((user = Verify(username, password, rmthost)) != NULL)
			{
				int i;
	
				 DEBUG("Verify OK\n");
				for (i = 0; i < sizeof(password); i++)
					password[i]=rand()%256;
			}
			else
			{
			 DEBUG("Verify FAIL\n");
				AlertDialog("Alert", "User name or password you\ntyped was incorrect!", "Ops", 0);
		
			 DEBUG("Verify INFO\n");
				password[0] = 0;
		
				RedrawElement(dialogPtr, 5);
		
				HandleLoginDialog(dialogPtr, menuPtr);
			}

			DisposeDialog(dialogPtr);
			 DEBUG("DisposeDialog()\n");
			ExitTinyAES();
			 DEBUG("ExitTinyAES()\n");

 			#ifdef DEBUG
			 ExitDebug();
			#endif

			if (user)
			{
				Logon(user, rmthost, -1);
			}

			fprintf(stderr, "Can't exec shell!");
			exit(0);
			break;

		case 9:
			if (1 == AlertDialog("Alert", "Do you realy want to\nREBOOT?", "Yes", "No", 0))
			{
	 			#ifdef DEBUG
				 ExitDebug();
				#endif

				DisposeDialog(dialogPtr);
				ExitTinyAES();

				printf("Rebooting system...");

				Shutdown(1);
			}
		
			HandleLoginDialog(dialogPtr, menuPtr);
			break;

		case 10:
			if (1 == AlertDialog("Alert", "Do you realy want to\nSHUTDOWN?", "Yes", "No", 0))
			{
	 			#ifdef DEBUG
				 ExitDebug();
				#endif

				DisposeDialog(dialogPtr);
				ExitTinyAES();

				printf("Halting system...\n");

				Shutdown(0);
			}
		
			HandleLoginDialog(dialogPtr, menuPtr);
			break;
	}
}

void BuildWelcomeString(char *string)
{	
	if (!Ssystem(-1, 0L, 0L))
	{
		char version[12];
		long i, info[] = {0, 0};
		
		for (i = 1; i >= 0; i--)
		{
			info[0] = Ssystem( i, 0L, 0L);
			strcat(string, (char *)info);
		}

		info[0] = Ssystem( 2, 0L, 0L);
		sprintf(version, " %d.%d.%d%c", 
		(int)(info[0]>>24)&0xFF, (int)(info[0]>>16)&0xFF, 
		(int)(info[0]>>8)&0xFF, (int)info[0]&0xFF); 
		strcat(string, version);
	}
}

static char *CleanLine(char *buffer)
{
	if (buffer != NULL)
	{
		int lineLength;
		int i;
    
		lineLength = strlen(buffer);

		for (i = 0; i < lineLength; i++)
		{
			if (buffer[i] != kSpace && buffer[i] != kTab)
				break;
		}

		if (i > 0 && i < lineLength)
		{
			lineLength -= i;
			memmove(buffer, buffer + i, lineLength);
		}

		for (i = lineLength; i > 0; i--)
		{
			int j = i - 1;

			if (buffer[j] != kSpace && buffer[j] != kTab)
				break;
		}

		buffer[i] = '\0';
	}

	return buffer;
}

sList *ReadConfig()
{
	FILE *file;

	char filename[] = "/etc/vlogin.conf";
	char *token;
	char *cleanLine;
	char line[1024];
	
	int lineCount = 0;

	sCommand *commands;
	sList *menuList = CreateList();
	sList *retVal = NULL;
	
	file = fopen(filename,"r");

	if (!feof(file))
	{
		do
		{
			char l_menuItem[256];
			char l_command[256];
			char l_argv[256];

			// get input line    
			cleanLine = CleanLine(fgets(line, sizeof(line), file));
			
			if (cleanLine == NULL) continue;

			lineCount++;

			// skip comments
			if (cleanLine[0] == '#')  continue;

			// get first token
		     token = CleanLine(strtok(line, "=\n\r"));

			if (token != NULL)
			{
				char *l_childargv[128];
				int inside	= 0;
				int tokenCount = 0;
				int tokenIndex = 0;
				int index		= 0;
				int argn 		= 0;
				
				l_menuItem[0]  = 0;
				l_command[0]   = 0;
				l_childargv[0] = 0; 
				l_argv[0]      = 0;

				for (index = 0; index < strlen(token); index++)
				{
					if (token[index] == kCommas)
					{
						inside ? inside-- : inside ++;
						index ++;
					}

					if (!inside && (token[index] == kSpace || token[index] == kTab))
					{
						if (tokenIndex)
						{
							tokenCount ++;

							if (tokenCount > 1)
							{
								char *argv = malloc(strlen(l_argv) + 1);
								strcpy(argv, l_argv);

								l_childargv[argn++] = argv;
							}
						}

						tokenIndex = 0;
					}
					else if (tokenCount == 0)
					{
						l_menuItem[tokenIndex ++] = token[index];
						l_menuItem[tokenIndex] = 0;
					}
					else if (tokenCount == 1)
					{
						l_command[tokenIndex] = l_argv[tokenIndex] = token[index];
						tokenIndex ++;
						l_command[tokenIndex] = l_argv[tokenIndex] = 0;
					}
					else if (tokenCount >= 2)
					{
						l_argv[tokenIndex++] = token[index];
						l_argv[tokenIndex] = 0;
					}
				}

				if (tokenCount)
				{
					char *argv = malloc(strlen(l_argv) + 1);
					strcpy(argv, l_argv);
					l_childargv[argn++] = argv;
					l_childargv[argn++] = NULL;
				}

				commands = malloc(sizeof(sCommand));
				
				commands->menuItem = malloc(sizeof(l_menuItem));
				strcpy(commands->menuItem, l_menuItem);

				commands->command = malloc(sizeof(l_command));
				strcpy(commands->command, l_command);

				argn++;

				commands->childargv = malloc(sizeof(char *) * argn);
				for (index = 0; index < argn; index ++)
					commands->childargv[index] = l_childargv[index];

				PushBack(menuList, commands);

				retVal = menuList;
			}

		} while (cleanLine != NULL);
	}

	fclose(file);
	
	return retVal;
}
