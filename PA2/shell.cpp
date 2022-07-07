#include <ctime>
#include <time.h>
#include <vector>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <sstream>
#include <limits.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <algorithm>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/types.h>




/*
Harshank Patel
UID: 527009145
PA2: Implementing a Linux Shell
*/




using namespace std;


std::string awkQuotesRemoval(string input){
    string ans = "";
    for(int i = 0; i < input.size(); i++){
        if(input.at(i) == '\''){
            //
        }
        else{
            ans += input.at(i);
        }
    }
    return ans;
}


std::string trim(std::string str) {
    while (!str.empty() && std::isspace(str.back()))
        str.pop_back();

    std::size_t pos = 0;
    while (pos < str.size() && std::isspace(str[pos]))
        ++pos;
    return str.substr(pos);
}


char **vec_to_char_array(vector<string> &position) {
    char **result = new char *[position.size() + 1];
    for (int i = 0; i < position.size(); i++) {
        result[i] = (char *)position[i].c_str();
    }
    result[position.size()] = NULL;
    return result;
}


vector<string> split(string str, char delim)
{

    vector<string> out;
    size_t start;
    size_t end = 0;

    while ((start = str.find_first_not_of(delim, end)) != string::npos)
    {
        end = str.find(delim, start);
        out.push_back(str.substr(start, end - start));
    }
    
    return out;
}


vector<string> specialAWKsplit(string str){
    // cerr <<"AWK Split Called \n\n";
    vector<string> ans;
    // cerr <<str.find("{")<<endl;
    // cerr <<str <<" is here boys" <<endl;

    std::size_t found = str.find("{");
    if (found != std::string::npos){
        // std:cerr << "found\n\n\n\n" << std::endl;
        int pos1 = str.find('{');
        vector<string> temp = split(str, '{');

        vector <string> temp2 = split(temp[0],'{');

        vector <string> bothPartsLeftRight = split(temp2[0],' ');


        string leftSide = bothPartsLeftRight[0];

        string rightSide = bothPartsLeftRight[1] + "{" + temp[1];

        // cerr <<trim(leftSide) <<" is the left side\n";
        // cerr <<trim(rightSide) <<" is the right side\n\n";
        ans.push_back(leftSide);
        ans.push_back(rightSide);
    }
    else{
        // std::cerr << "String not found\n\n\n\n" << std::endl; // If not found
        vector<string> temp = split(str, ' ');
        for(int i = 0; i < temp.size(); i++){
            ans.push_back(temp.at(i));
        }
    }
    return ans;
}


//Vector to keep track of all the running processes
vector<int> runningProcesses;


int main() {
    char prevWorkingDirectory[10000];
    getcwd(prevWorkingDirectory, sizeof(prevWorkingDirectory));
    dup2(0, 100);
    while (true) {
        bool awkProcess = false;
        dup2(100, 0);
        for (int i = 0; i < runningProcesses.size(); i++) {
            if (waitpid(runningProcesses[i], 0, WNOHANG) == runningProcesses[i]) {
                // cerr << "\nProcess: " << runningProcesses[i] << " ended" << endl;
                runningProcesses.erase(runningProcesses.begin() + i);
                i--;
            }
            else {
                if(runningProcesses.size() > 1){
                    cerr << "\nBackground Process Running:  " << runningProcesses[i] << endl;
                }
            }
        }



        //Make a system call to get the user name from the computer
        std::string username = getenv("USER");
        time_t rawtime;
        time (&rawtime);
        std::string t( ctime( &rawtime ) );
        cerr <<username <<" " <<t.substr( 0, t.length() -1  ) <<"$ ";

        
        
        string inputline;
        getline(cin, inputline); //get a line from standard input

        if (inputline == string("exit")) {
            cerr << "Bye!! End of shell" << endl;
            exit(0);
            break;
        }


        else if (trim(inputline).find("echo ") == 0){
            int pid = fork();
            if (pid == 0) {
                vector<string> parts;
                string command = inputline.substr (0,5);
                string instructions = inputline.substr (5);
                parts.push_back(trim(command));
                parts.push_back(trim(instructions));
                char **args = vec_to_char_array(parts);
                execvp(args[0], args);
            }
            waitpid(pid, 0, 0);
        }
        else{

            string s1 = "awk ";
            if (inputline.find(s1) != std::string::npos) {
                inputline = trim(awkQuotesRemoval(inputline));
                awkProcess = true;
            }



            vector<string> pipingString = split(inputline, '|');
            for (int i = 0; i < pipingString.size(); i++) {

                int fds[2];
                pipe(fds);
                int pid = fork();

                bool backGround = false;
                if (pipingString[i].find("&") >= 0) {
                    int pos = pipingString[i].find("&");
                    if (pos != -1) {
                        pipingString[i] = trim(pipingString[i].substr(0, pos - 1));
                        backGround = true;
                    }
                }



                //beginning of child process
                //child processs
                if (pid == 0) {
                    //dest,source
                    if (trim(inputline).find("cd ") == 0) {
                        //So Dash is fouond!
                        if (inputline.find("-") == inputline.size()-1) {
                            char currWorkingDirectory[10000];
                            getcwd(currWorkingDirectory, sizeof(currWorkingDirectory));

                            //Prev in the dash is same
                            if(strcmp(currWorkingDirectory,prevWorkingDirectory) == 0 ){
                                cerr <<"No Change Made because there is no previous directory in the history\n";
                            }
                            //Prev in dash is not same! Interchhange them
                            else{
                                char currWorkingDirectory[10000];
                                getcwd(currWorkingDirectory, sizeof(currWorkingDirectory));
                                chdir(prevWorkingDirectory);
                                strcpy ( prevWorkingDirectory,currWorkingDirectory);
                            }
                        }
                        else{
                            string dirName = trim(split(inputline,' ')[1]);
                            char currWorkingDirectory[10000];
                            getcwd(currWorkingDirectory, sizeof(currWorkingDirectory));
                            chdir(dirName.c_str());
                            if(strcmp(currWorkingDirectory,prevWorkingDirectory) == 0 ){
                            }else{
                                strcpy(prevWorkingDirectory,currWorkingDirectory);
                            }
                        }
                        continue;
                    }
                    
                    inputline = trim(pipingString[i]);

                    //I-O Redirect Writing to a FILE
                    int pos = inputline.find('>');
                    if (pos >= 0) {
                        string command = trim(inputline.substr(0, pos));
                        string fileName = trim(inputline.substr(pos + 1));

                        inputline = command;

                        int fd = open(fileName.c_str(), O_WRONLY | O_CREAT, S_IWUSR | S_IRUSR);

                        dup2(fd, 1);
                        close(fd);
                    }

                    //I-O Redirect Reading from a FILE
                    pos = inputline.find('<');
                    if (pos >= 0) {
                        string command = trim(inputline.substr(0, pos));
                        string fileName = trim(inputline.substr(pos + 1));

                        inputline = command;
                        int fd = open(fileName.c_str(), O_RDONLY | O_CREAT, S_IWUSR | S_IRUSR);
                        dup2(fd, 0);
                        close(fd);
                    }

                    //
                    //----//
                    if (i < pipingString.size() - 1) {
                        dup2(fds[1], 1);
                    }




                    vector<string> parts;
                    if(awkProcess == true){
                            parts = specialAWKsplit(trim(inputline));
                    }
                    else{
                        parts = split(inputline, ' ');
                    }


                    char **args = vec_to_char_array(parts);
                    execvp(args[0], args);
                }
                else {
                    if (!backGround) {
                        if (i == pipingString.size() - 1) {
                            waitpid(pid, 0, 0);
                        }
                        else {
                            runningProcesses.push_back(pid);
                        }
                    }
                    else {
                        runningProcesses.push_back(pid);
                    }
                    dup2(fds[0], 0);
                    close(fds[1]);
                }
            }
        }
    }
    exit(0);
    return 0;
}