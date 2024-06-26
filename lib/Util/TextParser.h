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
    // 성공 true, 실패 false 리터
    bool ChangeNamespace(std::string keySpace);


private:
    void SkipNoneCommand();
    void FillConfig();
    void GetWord(char* word);

    bool FindKeySpace(); // 다음 keyspace가 있으면 return true
    bool FindNextKey(); // 다음 key가 있으면 return true;
    void FindNextValue(char* value); // key가 있으면 다음 value는 있어야함


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

