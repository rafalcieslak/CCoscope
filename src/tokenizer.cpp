#include "tokenizer.h"
#include <iostream>

// Generated by flex.
int lexer(FILE* input, std::list<std::pair<token,fileloc>> &);

void tokenizer::scan(){
    if(tokenlist.size() >= 1){
        lookahead.push_back( tokenlist.front().first );
        previous.push_front( tokenlist.front().second ); // Store token location
        tokenlist.pop_front();
    }else{
        // No more tokens avaliable!
        lookahead.push_back(tkn_EOF);
    }
}

void tokenizer::prepare(std::string filename){

    FILE* fhandle = fopen(filename.c_str(), "r");
    lexer(fhandle, tokenlist);
    fclose(fhandle);

    // Print list of tokens for debugging purposes.
    for(auto &tkn : tokenlist){
        std::cout << tkn.first << " ";
    }
    std::cout << std::endl;
}

void tokenizer::syntaxerror(){
    // Lookahead is guaranteed to be non-empty in case of an error.
    auto listiter = std::next(previous.begin(), lookahead.size()-1);
    std::cout << "Syntax error, unexpected token " << lookahead.front() << ", line " << listiter->line << ", column " << listiter->column << ". \n" << std::endl;
}
