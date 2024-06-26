#define _CRT_SECURE_NO_WARNINGS
#include "TextParser.h"
#include <stdio.h>
#include <string>
using namespace std;


int TextParser::GetInt(string key)
{
	string val = _config[_keySpace][key];
	return std::stoi(val);
}

float TextParser::GetFloat(string key)
{
	string val = _config[_keySpace][key];
	return std::stof(val);
}

string TextParser::GetString(string key)
{
	return _config[_keySpace][key];
}

bool TextParser::LoadConfigFile(const char* fileName)
{
	if (fileName == nullptr)
		return false;

	//Init
	_config.clear();
	_configFile = fileName;
	_asteriskCommentStart = false;
	_keySpaceStart = false;


	FILE* configFile = fopen(_configFile, "rt");
	if (configFile == nullptr) {
		wprintf(L"open config file failed\n");
		return false;
	}

	fseek(configFile, 0, SEEK_END);
	long fEnd = ftell(configFile);
	_buffer = (char*)malloc(sizeof(char) * fEnd);
	if (_buffer == nullptr)
		return false;

	memset(_buffer, 0, fEnd);

	fseek(configFile, 0, SEEK_SET);

	fread(_buffer, sizeof(char), fEnd, configFile);
	fclose(configFile);

	char* origin = _buffer;
	//TODO: _config map ä���
	FillConfig();

	delete origin;

	for (const auto& outer_pair : _config) {
		cout << "Section: " << outer_pair.first << '\n';

		for (const auto& inner_pair : outer_pair.second) {
			cout << inner_pair.first << ": " << inner_pair.second << '\n';
		}

		cout << '\n';
	}


	return true;
}

void TextParser::FillConfig()
{
	while (FindKeySpace())
	{
		while (FindNextKey())
		{
			// false �����ϸ� FindKeySpace �ٽ�
		}
	}
}

bool TextParser::FindKeySpace()
{
	// : ���������� pass
	// �ּ��� �н��ؾ��ϳ� �ּ����� : ���ü��� �����ϱ�
	SkipNoneCommand();
	char keySpace[MAX_TEXT_PARSER_FILE_SIZE];
	memset(keySpace, 0, MAX_TEXT_PARSER_FILE_SIZE);
	if (*_buffer == ':')
	{
		_buffer++;
		GetWord(keySpace);
	}
	else {
		return false;
	}

	_keySpace = keySpace;
	return true;
}

bool TextParser::FindNextKey()
{
	//	�ּ� �н�
	//	���� �н�
	//	�ܾ� ã��
	//	������
	//  } �̰��̸�
	//  FindNextSpace�ؾ���
	SkipNoneCommand();
	if (*_buffer == '}')
	{
		_buffer++;
		return false;
	}
	else if (*_buffer == '{')
	{
		_buffer++;
	}

	char key[MAX_TEXT_PARSER_FILE_SIZE];
	memset(key, 0, MAX_TEXT_PARSER_FILE_SIZE);
	SkipNoneCommand();
	GetWord(key);
	FindNextValue(key);
	//char
	return true;
}

void TextParser::FindNextValue(char* key)
{
	//  ���� �н�
	//	= ã��
	//	�ܾ� ã��
	SkipNoneCommand();
	if (*_buffer == '=')
	{
		_buffer++;
		char value[MAX_TEXT_PARSER_FILE_SIZE];
		memset(value, 0, MAX_TEXT_PARSER_FILE_SIZE);
		SkipNoneCommand();
		GetWord(value);
		_config[_keySpace][key] = value;
	}
	else {
		wprintf(L"find value faield\n");
	}
}


void TextParser::GetWord(char* word)
{
	char* wordPtr = word;
	while (true) {

		if (*_buffer == 0x20 || *_buffer == 0x08 || *_buffer == 0x09 || *_buffer == 0x0a || *_buffer == 0x0d)
		{
			break;
		}

		*wordPtr = *_buffer;
		wordPtr++;
		_buffer++;
	}

	*wordPtr = '\0';
}

void TextParser::SkipNoneCommand()
{
	// �����̽�, ��, �����ڵ�, �ּ� ó��.
	while (true)
	{
		if (*_buffer == '/' && *(_buffer + 1) == '*')
		{
			*_buffer += 2;
			while (1)
			{
				if (*_buffer == '*' && *(_buffer + 1) == '/')
				{
					_buffer += 2;
					break;
				}
				_buffer++;
			}
		}
		else if (*_buffer == '/' && *(_buffer + 1) == '/')
		{
			*_buffer += 2;
			while (1)
			{
				if (*_buffer == '\n')
				{
					break;
				}
				_buffer++;
			}
			_buffer++;
		}
		else if (*_buffer == 0x20 || *_buffer == 0x08 || *_buffer == 0x09 ||
			*_buffer == 0x0a || *_buffer == 0x0d)
		{
			// spcae, back space, tab, line feed, carriage return
			_buffer++;
			continue;
		}
		else {
			break;
		}
	}
}

bool TextParser::ChangeNamespace(string keySpace)
{
	// key�� �ʿ� �����ϴ��� Ȯ��
	if (_config.find(keySpace) != _config.end()) {
		_keySpace = keySpace;
		return true;
	}
	else {
		return false;
	}
}