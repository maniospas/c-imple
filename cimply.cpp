#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iostream>
#include <unordered_set>

// g++ cimply.cpp -o cimply -O2 -std=c++20


std::vector<std::string> tokenize(const std::string& content) {
    std::vector<std::string> tokens;
    std::string token;
    for (char c : content) {
        if (std::isspace(c) || std::ispunct(c)) {
            if (!token.empty()) {
                tokens.push_back(token);
                token.clear();
            }
            if (std::ispunct(c)) {
                tokens.push_back(std::string(1, c));
            }
        } else {
            token += c;
        }
    }
    if (!token.empty()) {
        tokens.push_back(token);
    }
    return tokens;
}

std::vector<std::string> transformTokens(const std::vector<std::string>& tokens) {
    std::vector<std::string> newTokens;
    newTokens.emplace_back("#include <iostream>\n#include <memory>\n#define print(message) std::cout<<(message)<<std::endl\n");
    std::string fnName("");
    bool declaring = false;
    std::unordered_set<std::string> argNames;

    newTokens.emplace_back(
        "#include <stdexcept>\n"
        "\n"
        "template <typename T>\n"
        "class SafeSharedPtr {\n"
        "public:\n"
        "    SafeSharedPtr() = default;\n"
        "    explicit SafeSharedPtr(T* ptr) : ptr_(std::shared_ptr<T>(ptr)) {}\n"
        "    SafeSharedPtr(const std::shared_ptr<T>& ptr) : ptr_(ptr) {}\n"
        "    SafeSharedPtr(std::shared_ptr<T>&& ptr) : ptr_(std::move(ptr)) {}\n"
        "    SafeSharedPtr(std::nullptr_t) : ptr_(nullptr) {}\n"
        "    // Override dereference operator\n"
        "    T& operator*() const {\n"
        "        if (!ptr_) {\n"
        "            throw std::runtime_error(\"Dereferencing a null shared pointer!\");\n"
        "        }\n"
        "        return *ptr_;\n"
        "    }\n"
        "    T* operator->() const {\n"
        "        if (!ptr_) {\n"
        "            throw std::runtime_error(\"Accessing a null shared pointer!\");\n"
        "        }\n"
        "        return ptr_.get();\n"
        "    }\n"
        "    void unbind() {ptr_=nullptr;}\n"
        "    operator std::shared_ptr<T>() const {return ptr_;}\n"
        "    bool is_null() const {return !ptr_;}\n"
        "    void reset(T* ptr = nullptr) {ptr_.reset(ptr);}\n"
        "    std::shared_ptr<T> get() const {return ptr_;}\n"
        "private:\n"
        "    std::shared_ptr<T> ptr_;\n"
        "};\n"
        "template <typename T, typename... Args>\n"
        "SafeSharedPtr<T> make_safe_shared(Args&&... args) {\n"
        "    return SafeSharedPtr<T>(std::make_shared<T>(std::forward<Args>(args)...));\n"
        "}\n"
        "\n"
    );



    for(int i=0;i<tokens.size();++i) {
        if(tokens[i]=="fn") {
            if(i>=tokens.size()-4) {
                std::cerr << "`fn` function declaration was not complete" << std::endl;
                exit(1);
            }
            if(tokens[i+1]=="main") {
                newTokens.emplace_back("int");
                continue;
            }
            newTokens.emplace_back("auto");
            fnName = tokens[i+1];
            declaring = true;
            argNames.clear();
            continue;
        }
        if(declaring && tokens[i]==")") 
            declaring = false; // end declaration but continue normally
        
        if(tokens[i]=="auto") {
            std::cerr << "`auto` is not allowed. Use `var` to declare variables or `fn` to declare functions." << std::endl;
            exit(1);
        }
        if(tokens[i]=="void") {
            std::cerr << "`void` is not allowed." << std::endl;
            exit(1);
        }
        if(tokens[i]=="delete") {
            std::cerr << "`delete` is not allowed. Use `unbind` to let the memory handler process how the value should best be removed from this context." << std::endl;
            exit(1);
        }
        if(tokens[i]=="nullptr") {
            std::cerr << "`nullptr` is not allowed. If your are trying to do `varname=nullptr;`, you may consider `unbind varname;` instead  to let the memory handler process how the value should best be removed from this context." << std::endl;
            exit(1);
        }
        if(tokens[i]=="unbind") {
            if(i>=tokens.size()-1) 
                std::cerr << "Nothing to delete";
            int pos = i+1;
            while(pos<tokens.size()) {
                if(tokens[pos]==";") {
                    newTokens.emplace_back(".");
                    newTokens.emplace_back("unbind()");
                    newTokens.emplace_back(";");
                    break;
                }
                newTokens.emplace_back(tokens[pos]);
                pos++;
            }
            i = pos;
            continue;
        }
        if(tokens[i]=="this") {
            std::cerr << "`this` is not allowed. Use `self.` (note the fullstop) to access the struct's own fields." << std::endl;
            exit(1);
        }
        if(tokens[i]=="&") {
            std::cerr << "`&` is not allowed. Construct `shared[type]` objects" << std::endl;
            exit(1);
        }
        /*if(tokens[i]=="private") {
            std::cerr << "`private` is not allowed. All struct members must be public." << std::endl;
            exit(1);
        }
        if(tokens[i]=="public") {
            std::cerr << "`public` is not allowed. All struct members are already public." << std::endl;
            exit(1);
        }*/
        if(tokens[i][0]=='#') {
            std::cerr << "Preprocessor commands are not allowed." << std::endl;
            exit(1);
        }
        if(tokens[i]=="class") {
            std::cerr << "`class` is not allowed. Use `struct` instead." << std::endl;
            exit(1);
        }
        if(tokens[i]=="const") {
            std::cerr << "`const` is not allowed as it is automatically applied." << std::endl;
            exit(1);
        }
        if(tokens[i]=="-" && i<tokens.size()-1 && tokens[i+1]==">") {
            std::cerr << "`->` is not allowed, as it is automatically inferred. Use `.` instead." << std::endl;
            exit(1);
        }
        if(tokens[i]==":") {
            std::cerr << "`:` is not a valid syntax." << std::endl;
            exit(1);
        }
        if(tokens[i]=="@") {
            newTokens.emplace_back(":");
            newTokens.emplace_back(":");
            continue;
        }
        if(tokens[i]==".") {
            newTokens.emplace_back("->");
            continue;
        }
        if(tokens[i]=="struct") {
            if(i>=tokens.size()-3) {
                std::cerr << "`struct` definition is incomplete" << std::endl;
                exit(1);
            }
            if(tokens[i+2]!="{") {
                std::cerr << "Invalid `struct `definition" << std::endl;
                exit(1);
            }
            newTokens.emplace_back("struct");
            newTokens.emplace_back(tokens[i+1]);
            newTokens.emplace_back("{");
            newTokens.emplace_back(tokens[i+1]+"* operator->() {return this;} // optimized away by -O2 \n");
            newTokens.emplace_back("const "+tokens[i+1]+"* operator->() const {return this;} // optimized away by -O2 \n");
            i += 2;
            continue;
        }
        if(tokens[i]=="var") {
            if(declaring) {
                std::cerr << "Explicit types are always expected as function arguments." << std::endl;
                exit(1);
            }
            newTokens.emplace_back("auto");
            if(i<tokens.size()-1)
                argNames.insert(tokens[i+1]);
            continue;
        }
        if(tokens[i]=="self") {
            if(i<tokens.size()-1 && tokens[i+1]==".") {
                newTokens.emplace_back("this ->");
                i += 1;
                continue;
            }
            else {
                std::cerr << "`self` must be followed by `.` and cannot be returned" << std::endl;
                exit(1);
            }
        }
        if(tokens[i]=="shared") {
            if(i>=tokens.size()-1 || tokens[i+1]!="[") {
                std::cerr << "`shared` must be followed by `[`" << std::endl;
                exit(1);
            }
            int pos = i+2;
            int depth = 1;
            if(i && (tokens[i-1]=="=" || tokens[i-1]=="return" || (tokens[i-1]=="(" && !declaring)))
                newTokens.emplace_back("make_safe_shared");
            else {
                // when declaring function inputs, they should be const and & to avoid creating redundant shared ptrs
                if(declaring)
                    newTokens.emplace_back("const");
                newTokens.emplace_back("SafeSharedPtr");
            }
            newTokens.emplace_back("<");
            while(pos<tokens.size()) {
                if(tokens[pos]=="[")
                    depth++;
                if(tokens[pos]=="]")
                    depth--;
                if(depth==0)
                    break;
                if(tokens[pos]=="void") {
                    std::cerr << "`void` is not allowed." << std::endl;
                    exit(1);
                }
                if(tokens[pos]=="auto") {
                    std::cerr << "`auto` is not allowed. Use `var` to declare variables or `fn` to declare functions. However, here you are in a function signature, where even `var` is disallowed." << std::endl;
                    exit(1);
                }
                if(tokens[pos]=="var") {
                    std::cerr << "Explicit types are always expected as function arguments." << std::endl;
                    exit(1);
                }
                newTokens.emplace_back(tokens[pos]);
                pos++;
            }
            if(depth!=0){
                std::cerr << "`[` has no matching `]`" << std::endl;
                exit(1);
            }
            i = pos;
            newTokens.emplace_back(">");
            if(declaring) // close the const & declaration
                newTokens.emplace_back("&");
            if(declaring && i<tokens.size()-1)
                argNames.insert(tokens[i+1]);
            continue;
        }
        if(declaring && i<tokens.size()-1 && (tokens[i+1]=="," || (tokens[i+1]==")" && tokens[i]!="(")) && newTokens[newTokens.size()-1]!="&") {
            newTokens.emplace_back("&");
        } 

        newTokens.emplace_back(tokens[i]);
    }

    return std::move(newTokens);
}

void processFile(const std::string& filename) {
    std::ifstream infile(filename);
    if (!infile.is_open()) {
        std::cerr << "Could not open file: " << filename << std::endl;
        return;
    }

    std::stringstream buffer;
    buffer << infile.rdbuf();
    infile.close();

    std::vector<std::string> tokens = tokenize(buffer.str());
    tokens = transformTokens(tokens);

    std::string output_filename = filename.substr(0, filename.find_last_of('.')) + ".cpp";
    std::ofstream outfile(output_filename);
    if (!outfile.is_open()) {
        std::cerr << "Could not open file for writing: " << output_filename << std::endl;
        return;
    }

    std::string prefix("");
    for (int i=0;i<tokens.size()-1;++i) {
        std::string& token = tokens[i];
        std::string& nextToken = tokens[i+1];
        outfile << token;
        if(token.size()>1 && nextToken.size()>1)
            outfile << ' ';
        else if(std::isalnum(static_cast<unsigned char>(token[0])) && std::isalnum(static_cast<unsigned char>(nextToken[0])))
            outfile << ' ';
        if(token.size() && token[token.size()-1]=='{')
            prefix += "   ";
        if(token.size() && (token[token.size()-1]=='{' || token[0]=='}' || token[token.size()-1]==';' || token[0]=='#')) {
            outfile << '\n';
            if(nextToken.size() && nextToken[0]=='}') {
                prefix.pop_back();
                prefix.pop_back();
                prefix.pop_back();
            }
            outfile << prefix;
        }
        else if(token.size() && token[token.size()-1]=='\n') {
            outfile << prefix.substr(1);
        }
    }
    outfile << tokens[tokens.size()-1];

    outfile.close();
    std::cout << "File processed and saved as: " << output_filename << std::endl;

    
    std::string executable_name = filename.substr(0, filename.find_last_of('.'));
    std::string compile_command = "g++ " + output_filename + " -o "+executable_name+" -O2 -std=c++20 && ./"+executable_name;
    if (std::system(compile_command.c_str()) != 0) 
        std::cerr << "Failed to compile or run the generated code." << std::endl;
    else
        std::cout << "Execution finished" << std::endl;

}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <source.cm>" << std::endl;
        return 1;
    }
    processFile(argv[1]);
    return 0;
}
