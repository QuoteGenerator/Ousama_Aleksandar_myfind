#include <iostream>
#include <vector>
#include <unistd.h>
#include <filesystem>
#include <strings.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <limits.h>
#include <string>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cerrno>


void pathSearcher(char *searchPath, const std::string& inputFilename, std::vector<std::string>& foundPaths, std::vector<std::string>& foundFileNames, bool caseInsensitive, bool recursiveOption){
    if(recursiveOption == true){
        for(const auto& entry : std::filesystem::recursive_directory_iterator(searchPath)){
            const std::string searchedFilename = entry.path().filename().string();
            if(!std::filesystem::is_regular_file(entry)){
                continue; //überspringe Ordner.
            }
            
            if (searchedFilename.length() != inputFilename.length())
            {
                continue; //automatisch falsch
            }
            
            if(caseInsensitive == true){
                if(strncasecmp(searchedFilename.c_str(), inputFilename.c_str(), inputFilename.length()) == 0){
                    foundFileNames.emplace_back(searchedFilename);
                    foundPaths.emplace_back(std::filesystem::absolute(entry.path()).string());
                }
            }
            else if (searchedFilename == inputFilename){
                foundFileNames.emplace_back(searchedFilename);
                foundPaths.emplace_back(std::filesystem::absolute(entry));
            } 
        }
    }
    else{
        for(const auto& entry : std::filesystem::directory_iterator(searchPath)){
            const std::string searchedFilename = entry.path().filename().string();
            if(!std::filesystem::is_regular_file(entry)){
                continue;
            }
            
            if (searchedFilename.length() != inputFilename.length())
            {
                continue; //automatisch falsch
            }
            
            if(caseInsensitive == true){                
                if(strncasecmp(searchedFilename.c_str(), inputFilename.c_str(), inputFilename.length()) == 0){
                    foundFileNames.emplace_back(searchedFilename);
                    foundPaths.emplace_back(std::filesystem::absolute(entry.path()).string());
                }
            } else if (searchedFilename == inputFilename){
                foundFileNames.emplace_back(searchedFilename);
                foundPaths.emplace_back(std::filesystem::absolute(entry.path()).string());
            }   
        }
    }
}

void writeIntoPipe(int fileDescriptor, const std::vector<std::string>& foundPaths, const std::vector<std::string>& foundFilenames){
    FILE* writing;
    if((writing = fdopen(fileDescriptor, "w")) == NULL){
        std::cerr << "Failed to write\n";
        _exit(EXIT_FAILURE); // EXIT FAILURE ist einfach 1
    }

    //schreibt in die pipe
    if(foundFilenames.empty() || foundPaths.empty()){
        std::fprintf(writing, "%d: File not found\n", (int)getpid());
        std::fflush(writing);
        fclose(writing);
        return;
    }

    for(size_t i = 0; i < foundFilenames.size(); ++i){
        std::fprintf(writing, "%d: %s: %s\n", (int)getpid(), foundFilenames[i].c_str(), foundPaths[i].c_str());
        std::fflush(writing); //flushed den buffer
    }
    fclose(writing);//schließt file descriptor vom Kind
}


/* 
    -   Für jeden Dateinamen erstellt der Hauptprozess mithilfe von fork() einen Kindprozess.
    -   Jeder Kindprozess sucht nach der Datei und schreibt seine Ergebnisse in eine Pipe. 
    -   Das Elternprozess liest aus der Pipe und gibt diese aus (die Ergebnisse).
*/

int main(int argc, char *argv[]){

    std::vector<std::string> foundPaths;
    std::vector<std::string> foundFileNames;
    int opt; 
    bool caseInsensitive = false, recursiveOption = false; 

    while((opt = getopt(argc, argv, "iR")) != -1){
        switch (opt)
        {
            case 'i':
                caseInsensitive = true;
                break;
            case 'R':
                recursiveOption = true;
                break;
            
            default: 
                break;
        }
    }
    
    int fileDescriptor[2];
    char buffer[PIPE_BUF];
    FILE* reading;
    char* searchDir = argv[optind];
    std::string inputFilename; // Wird in der for schleife zugewiesen (direkt darunter)

    if (!std::filesystem::exists(searchDir) || !std::filesystem::is_directory(searchDir)) {
        std::cerr << "Directory not found: " << searchDir << "\n";
        return EXIT_FAILURE; 
    }

    //pipe wird geöffnet
    if(pipe(fileDescriptor) < 0){
        std::cerr <<"pipe\n";
        return EXIT_FAILURE;
    }

    
    for (int i = optind+1; i < argc; i++){
        pid_t PID = fork();
        
        if (PID == -1){
            std::cerr << "Failure in creating a child process!";
            //warten auf davor erstellte child processes
            pid_t result;
            while ((result = waitpid(-1, nullptr, 0)) > 0) {
                // auf kinder warten, muss leer sein
            }
            
            return EXIT_FAILURE;
        } 
        else if (PID == 0){
            close(fileDescriptor[0]); //Kinder brauchen lesen nicht
            inputFilename = argv[i];
            pathSearcher(searchDir, inputFilename, foundPaths, foundFileNames, caseInsensitive, recursiveOption);
            writeIntoPipe(fileDescriptor[1], foundPaths, foundFileNames);
            return EXIT_SUCCESS; //EXIT SUCCESS ist einfach 0
        }
    }
    
    close(fileDescriptor[1]);// Parent braucht nicht schreiben 

    if((reading = fdopen(fileDescriptor[0], "r")) == NULL){
        std::cerr << "Failed to read\n";
        return EXIT_FAILURE;
    }

    while(fgets(buffer, PIPE_BUF, reading) != NULL){
        fputs(buffer, stdout);
        fflush(stdout);
    }

    fclose(reading);// schließt file descriptor

    //wartet auf alle Kinder
    pid_t result;
    while ((result = waitpid(-1, nullptr, 0)) > 0) {
        //auf kinder warten, muss leer sein
    }
}

