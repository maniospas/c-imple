#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iostream>
#include <unordered_set>
#include <unordered_map>
#include <cctype>
#include <stdexcept>

// g++ cimply.cpp -o cimply -O2 -std=c++20

std::vector<std::string> tokenize(const std::string& content) {
    std::vector<std::string> tokens;
    std::string token;
    for (char c : content) {
        if (std::isspace(static_cast<unsigned char>(c)) || std::ispunct(static_cast<unsigned char>(c))) {
            if (!token.empty()) {
                tokens.push_back(token);
                token.clear();
            }
            if (std::ispunct(static_cast<unsigned char>(c))) {
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


// Primitive types that should not be converted to const Type&
static std::unordered_set<std::string> primitiveTypes = {"int", "double", "bool"};

// Recursive function to parse types with nesting
std::string parseType(const std::vector<std::string>& tokens, int& pos, const std::unordered_set<std::string>& conceptNames, bool declaring, bool& isConstructorCall) {
    if (pos >= tokens.size()) {
        throw std::runtime_error("Unexpected end of tokens while parsing type.");
    }
    
    std::string type = "";
    while (pos < tokens.size()) {
        std::string current = tokens[pos];

        if (current == "vector" || current == "shared") {
            std::string templateName;
            bool isShared = (current == "shared");
            pos++; // Move past 'vector' or 'shared'

            if (pos >= tokens.size() || tokens[pos] != "[") {
                throw std::runtime_error("Expected '[' after " + current);
            }
            pos++; // Move past '['

            // Start parsing the nested type
            std::string nestedType = parseType(tokens, pos, conceptNames, false, isConstructorCall);

            // Expect closing ']'
            if (pos >= tokens.size() || tokens[pos] != "]") {
                throw std::runtime_error("Expected ']' after type parameters.");
            }
            pos++; // Move past ']'

            // Check if the next token is '(' to determine if it's a constructor call
            int lookahead = pos;
            while (lookahead < tokens.size() && tokens[lookahead] == " ") {
                lookahead++;
            }
            bool followedByParenthesis = (lookahead < tokens.size() && tokens[lookahead] == "(");

            if (isShared && followedByParenthesis) {
                // It's a constructor call
                templateName = "make_safe_shared";
                isConstructorCall = true;
            } else {
                // Regular type usage
                if (isShared)
                    templateName = "SafeSharedPtr";
                else
                    templateName = "SafeVector";
            }

            type += templateName + "<" + nestedType + ">";
        }
        else if (current == "[") {
            pos++; // Move past '['
            std::string nestedType = parseType(tokens, pos, conceptNames, false, isConstructorCall);
            if (pos >= tokens.size() || tokens[pos] != "]") {
                throw std::runtime_error("Expected ']' after '['");
            }
            pos++; // Move past ']'
            type += "[" + nestedType + "]";
        }
        else if (current == ",") {
            pos++; // Move past ','
            type += ", ";
        }
        else if (current == "]" || current == ")" || current == ";") {
            // End of type
            break;
        }
        else if (current == "const" || current == "&" || current == "void") {
            std::runtime_error("`"+current+"` are reserved for internal usage.");
        }
        else {
            // Base type
            if (current == "string") 
                current = "std::string";
        
            // If declaring and the type is a function argument, append '&' unless it's a primitive type
            if (declaring && !isConstructorCall) {
                if (primitiveTypes.find(type) == primitiveTypes.end()) {
                    // Not a primitive type, convert to const Type&
                    type = "const " + type + "&";
                }
            }

            type += current;
            pos++;
        }

        // Handle multiple tokens for base types (e.g., 'unsigned int')
        if (pos < tokens.size() && tokens[pos] == "int" && tokens[pos - 1] == "unsigned") {
            type += " " + tokens[pos];
            pos++;
        }
    }

    return type;
}

std::vector<std::string> transformTokens(const std::vector<std::string>& tokens) {
    std::vector<std::string> newTokens;
    newTokens.emplace_back("#include <iostream>\n#include <vector>\n#include <memory>\n#include <string>\n#define print(message) std::cout<<(message)<<std::endl\n");
    std::string fnName("");
    bool declaring = false;
    bool inConcept = false;
    std::unordered_set<std::string> argNames;
    std::unordered_set<std::string> conceptNames;

    newTokens.emplace_back(
        "#include <stdexcept>\n"
        "\n"
        "template <typename T>\n"
        "class SafeSharedPtr {\n"
        "public:\n"
        "    SafeSharedPtr() = default;\n"
        "    explicit SafeSharedPtr(T* ptr) : ptr_(std::shared_ptr<T>(ptr)) {}\n"
        "    explicit SafeSharedPtr(const std::shared_ptr<T>& ptr) : ptr_(ptr) {}\n"
        "    explicit SafeSharedPtr(std::shared_ptr<T>&& ptr) : ptr_(std::move(ptr)) {}\n"
        "    SafeSharedPtr(std::nullptr_t) : ptr_(nullptr) {}\n"
        "    operator T&() const {return *ptr_;} \n"
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
        "};\n\n"
        "template <typename T, typename... Args>\n"
        "SafeSharedPtr<T> make_safe_shared(Args&&... args) {\n"
        "    return SafeSharedPtr<T>(std::make_shared<T>(std::forward<Args>(args)...));\n"
        "}\n"
        "\n"
    );


    newTokens.emplace_back(
        "template <typename T>\n"
        "class SafeVector {\n"
        "private:\n"
        "    std::vector<T> data;\n"
        "\n"
        "public:\n"
        "    SafeVector() = default;\n"
        "    SafeVector(int size) : data(size) {}\n"
        "    SafeVector(std::initializer_list<T> init) : data(init) {}\n"
        "    size_t size() const { return data.size(); }\n"
        "    SafeVector* operator->() {return this;} // optimized away by -O2 \n"
        "    const SafeVector* operator->() const {return this;} // optimized away by -O2 \n"
        "    T& operator[](size_t index) {\n"
        "        if (index >= data.size()) throw std::out_of_range(\"Index \"+std::to_string(index)+\" casted from negative int or out of bounds in `vector` with \"+std::to_string(data.size())+\" elements\");\n"
        "        return data[index];\n"
        "    }\n"
        "    const T& operator[](size_t index) const {\n"
        "        if (index >= data.size()) throw std::out_of_range(\"Index \"+std::to_string(index)+\" casted from negative int or out of bounds in `vector` with \"+std::to_string(data.size())+\" elements\");\n"
        "        return data[index];\n"
        "    }\n"
        "    void set(size_t index, const T& value) {\n"
        "        if (index >= data.size()) throw std::out_of_range(\"Index \"+std::to_string(index)+\" casted from negative int or out of bounds in `vector` with \"+std::to_string(data.size())+\" elements\");\n"
        "        data[index] = value;\n"
        "    }\n"
        "    void pop() {\n"
        "        if (data.empty()) throw std::out_of_range(\"Pop from empty SafeVector\");\n"
        "        data.pop_back();\n"
        "    }\n"
        "    void reserve(size_t size) { data.reserve(size); }\n"
        "    void push(const T& value) { data.push_back(value); }\n"
        "    void clear() { data.clear(); }\n"
        "    bool empty() const { return data.empty(); }\n"
        "};\n\n"
    );


    for(int i=0;i<tokens.size();++i) {
        if(tokens[i]=="func") {
            inConcept = false;
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
            newTokens.emplace_back(fnName);
            declaring = true;
            argNames.clear();
            i += 1;
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
            std::cerr << "`nullptr` is not allowed. If you are trying to do `varname=nullptr;`, you may consider `unbind varname;` instead to let the memory handler process how the value should best be removed from this context." << std::endl;
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
        if(tokens[i]=="type") {
            if(i>=tokens.size()-3) {
                std::cerr << "`type` definition is incomplete" << std::endl;
                exit(1);
            }
            if(tokens[i+2]!="{") {
                std::cerr << "Invalid `type` definition" << std::endl;
                exit(1);
            }
            newTokens.emplace_back("template");
            newTokens.emplace_back("<");
            newTokens.emplace_back("typename T");
            newTokens.emplace_back(">");
            newTokens.emplace_back("concept");
            newTokens.emplace_back(tokens[i+1]);
            newTokens.emplace_back("=");
            newTokens.emplace_back("requires");
            newTokens.emplace_back("(");
            newTokens.emplace_back("T");
            newTokens.emplace_back("self");
            newTokens.emplace_back(")");
            newTokens.emplace_back("{");
            conceptNames.insert(tokens[i+1]);
            inConcept = false;
            i += 2;
            continue;
        }
        if(tokens[i]=="struct") {
            inConcept = false;
            if(i>=tokens.size()-3) {
                std::cerr << "`struct` definition is incomplete" << std::endl;
                exit(1);
            }
            if(tokens[i+2]!="{") {
                std::cerr << "Invalid `struct` definition" << std::endl;
                exit(1);
            }
            newTokens.emplace_back("struct");
            newTokens.emplace_back(tokens[i+1]);
            newTokens.emplace_back("{");
            newTokens.emplace_back(tokens[i+1]+"* operator->() {return this;} // optimized away by -O2 \n");
            newTokens.emplace_back("const "+tokens[i+1]+"* operator->() const {return this;} // optimized away by -O2 \n");
            newTokens.emplace_back(tokens[i+1]+"(const "+tokens[i+1]+"& other) = default; \n");
            newTokens.emplace_back(tokens[i+1]+"("+tokens[i+1]+"&& other) = default; \n");
            i += 2;
            continue;
        }
        if(tokens[i]=="exists") {
            int depth = 1;
            int pos = i+2;
            std::string type("");
            while(pos<tokens.size()) {
                if(tokens[pos]=="[")
                    depth++;
                if(tokens[pos]=="]")
                    depth--;
                if(depth==0)
                    break;
                type += tokens[pos];
                if(tokens[pos]=="[")
                    depth++;
                pos++;
            }
            pos++;

            depth = 0;
            newTokens.emplace_back("{ ");
            while(pos<tokens.size()) {
                if(tokens[pos]=="{")
                    depth++;
                if(tokens[pos]=="}")
                    depth--;
                if(tokens[pos]==";" && depth==0)
                    break;
                if(tokens[pos]==".")
                    newTokens.emplace_back("->");
                else
                    newTokens.emplace_back(tokens[pos]);
                pos++;
            }
            if(pos==tokens.size()) {
                std::cerr << "Never terminated the `exists` statement with `;`." << std::endl;
                exit(1);
            }
            newTokens.emplace_back(" }");
            newTokens.emplace_back("->");
            newTokens.emplace_back("std::convertible_to");
            newTokens.emplace_back("<");
            newTokens.emplace_back(type);
            newTokens.emplace_back(">");
            newTokens.emplace_back(";");
            i = pos;
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
        if(conceptNames.find(tokens[i])!=conceptNames.end() && declaring) {
            newTokens.push_back("const");
            newTokens.push_back(tokens[i]);
            newTokens.push_back("auto");
            newTokens.push_back("&");
            continue;
        }
        if(tokens[i]=="shared" || tokens[i]=="vector") {
            // Start parsing the type
            int posCopy = i;
            bool isConstructorCall = false;
            try {
                std::string parsedType = parseType(tokens, posCopy, conceptNames, declaring, isConstructorCall);
                newTokens.emplace_back(parsedType);
                i = posCopy - 1; // Adjust for loop increment
            }
            catch (const std::exception& e) {
                std::cerr << "Type parsing error at token " << i << ": " << e.what() << std::endl;
                exit(1);
            }
            continue;
        }
        if(declaring 
            && tokens[i]!="," 
            && tokens[i]!="("  
            && tokens[i]!=")" 
            && primitiveTypes.find(tokens[i]) == primitiveTypes.end()
            && i<tokens.size()-1 && tokens[i+1]!="," && tokens[i+1]!="=" && tokens[i+1]!=")"
            && i && tokens[i-1]!="=") {
            //newTokens.emplace_back("const");
            newTokens.emplace_back(tokens[i]);
            newTokens.emplace_back("&");
            continue;
        }
        /*if(declaring && (tokens[i]=="," || (tokens[i]==")" && tokens[i-1]!="("))) {
            // Continue to next token
            newTokens.emplace_back(";");
            newTokens.emplace_back(tokens[i]);
            continue;
        }*/

        // Replace 'string' with 'std::string' and 'boolean' with 'bool'
        if(tokens[i]=="string") {
            newTokens.emplace_back("std::string");
            continue;
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
        else if(token.size()==0 || nextToken.size()==0) {}
        else if(std::isalnum(static_cast<unsigned char>(token[0])) && std::isalnum(static_cast<unsigned char>(nextToken[0])))
            outfile << ' ';
        if(token.size() && token[token.size()-1]=='{')
            prefix += "   ";
        if(token.size() && (token[token.size()-1]=='{' || token[0]=='}' || token[token.size()-1]==';' || token[0]=='#')) {
            outfile << '\n';
            if(nextToken.size() && nextToken[0]=='}') {
                if (prefix.size() >= 3)
                    prefix.resize(prefix.size() - 3);
            }
            outfile << prefix;
        }
        else if(token.size() && token[token.size()-1]=='\n') {
            if(prefix.size())
                outfile << prefix.substr(1);
        }
    }
    if (!tokens.empty())
        outfile << tokens.back();

    outfile.close();
    std::cout << "Transpiled: " << output_filename << std::endl;


    std::string executable_name = filename.substr(0, filename.find_last_of('.'));
    std::string compile_command = "g++ " + output_filename + " -o "+executable_name+" -O2 -std=c++20 && ./" + executable_name;
    if (std::system(compile_command.c_str()) != 0) 
        std::cerr << "Failed to compile or run the generated code." << std::endl;
    //else
    //    std::cout << "Execution finished" << std::endl;

}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <source.cm>" << std::endl;
        return 1;
    }
    processFile(argv[1]);
    return 0;
}
