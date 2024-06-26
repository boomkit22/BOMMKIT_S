#pragma once
#include <iostream>
#include <map>

#define MAX_TEXT_PARSER_FILE_SIZE 64


class TextParser
{
public:
    bool LoadConfigFile(const char* fileName);
    int GetInt(std::string key);
    float GetFloat(std::string key);

    std::string GetString(std::string key);
    // ���� true, ���� false ����
    bool ChangeNamespace(std::string keySpace);


private:
    void SkipNoneCommand();
    void FillConfig();
    void GetWord(char* word);

    bool FindKeySpace(); // ���� keyspace�� ������ return true
    bool FindNextKey(); // ���� key�� ������ return true;
    void FindNextValue(char* value); // key�� ������ ���� value�� �־����


private:
    const char* _configFile = "config.txt";
    std::string _keySpace;
    char* _buffer = nullptr;
    //key : keySpace
    //value : {
    //         key : key
    //         Value : value
    //         } 
    // 
    bool _asteriskCommentStart = false;
    bool _keySpaceStart = false;
    std::map<std::string, std::map<std::string, std::string>> _config;
};

