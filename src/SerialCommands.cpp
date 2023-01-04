/*
* Project: SerialCommands.cpp
* Description: This is a library for handling commands from a Stream object
* Author: Gunnar L. Larsen
* Date: 12/20/2022
*/

#include "SerialCommands.h"
#include "log.h"

void SerialCommands::AddCommand(SerialCommand* command)
{
#ifdef SERIAL_COMMANDS_DEBUG
	log("Adding #%d cmd=[%s]", commands_count_, command->command);
#endif
	if (commands_count_ < MAX_COMMANDS-1) {
	    commands_ptr[commands_count_++] = command;
	} else {
#ifdef SERIAL_COMMANDS_DEBUG
    	log("Max number of supported commands exceeded!"); 
#endif
	}
}

SERIAL_COMMANDS_ERRORS SerialCommands::ReadSerial()
{
	if (serial_ == NULL)
	{
		return SERIAL_COMMANDS_ERROR_NO_SERIAL;
	}

	while (serial_->available() > 0)
	{
		int ch = serial_->read();
#ifdef SERIAL_COMMANDS_DEBUG
		log("Read: bufLen=%d bufPos=%d termPos=%d ch=%c", buffer_len_, buffer_pos_, term_pos_, ch);
#endif
		if (ch <= 0)
		{
			continue;
		}

		if (buffer_pos_ < buffer_len_)
		{
			buffer_[buffer_pos_++] = ch;
		}
		else
		{
#ifdef SERIAL_COMMANDS_DEBUG
			log("Buffer full");
#endif
			ClearBuffer();
			return SERIAL_COMMANDS_ERROR_BUFFER_FULL;
		}

		if (term_[term_pos_] != ch)
		{
			term_pos_ = 0;
			continue;
		}

		if (term_[++term_pos_] == 0)
		{
			buffer_[buffer_pos_ - strlen(term_)] = '\0';
#ifdef SERIAL_COMMANDS_DEBUG
			log("Received: [%s]", buffer_);
#endif
			char* command = strtok_r(buffer_, delim_, &last_token_);
			if (command != NULL)
			{
				boolean matched = false;
				int cx;
				SerialCommand* cmd;
				for (cx = 0; cx < commands_count_; cx++)
				{
#ifdef SERIAL_COMMANDS_DEBUG
					log("Comparing [%s] to [%s]", command, commands_ptr[cx]->command);
#endif
					if (strncmp(command, commands_ptr[cx]->command, strlen(commands_ptr[cx]->command) + 1) == 0)
					{
#ifdef SERIAL_COMMANDS_DEBUG
						log("Matched #%d", cx);
#endif
						commands_ptr[cx]->function(this);
						matched = true;
						break;
					}
					
				}
				if (!matched && default_handler_ != NULL)
				{
					(*default_handler_)(this, command);
				}
			}

			ClearBuffer();
		}
	}

	return SERIAL_COMMANDS_SUCCESS;
}


Stream* SerialCommands::GetSerial()
{
	return serial_;
}

void SerialCommands::AttachSerial(Stream* serial)
{
	serial_ = serial;
}

void SerialCommands::DetachSerial()
{
	serial_ = NULL;
}

void SerialCommands::SetDefaultHandler(void(*function)(SerialCommands*, const char*))
{
	default_handler_ = function;
}

void SerialCommands::ClearBuffer()
{
	buffer_[0] = '\0';
	buffer_pos_ = 0;
	term_pos_ = 0;
}

char* SerialCommands::Next()
{
	return strtok_r(NULL, delim_, &last_token_);
}
