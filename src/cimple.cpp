#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iostream>
#include <unordered_set>
#include <unordered_map>
#include <cctype>
#include <stdexcept>
//#include "src/cget.h"


bool isNumber(const std::string& str) {
    std::istringstream ss(str);
    double d;
    char c;
    // Try to parse as a double and check for leftover characters
    return ss >> d && !(ss >> c);
}

// g++ src/cimple.cpp -o cimple -O2 -std=c++20
std::vector<std::string> tokenize(const std::string& content) {
    std::vector<std::string> tokens;
    std::string token;
    bool in_string = false;

    for (size_t i = 0; i < content.size(); ++i) {
        char c = content[i];

        // Check for start of line comment
        if (!in_string && c == '/' && i + 1 < content.size() && content[i + 1] == '/') {
            // Skip until end of line
            while (i < content.size() && content[i] != '\n') {
                ++i;
            }
            continue;
        }

        // Check for start or end of a quoted string
        if (c == '"' && (i == 0 || content[i - 1] != '\\')) {
            if (in_string) {
                token += c;
                tokens.push_back(token);
                token.clear();
                in_string = false;
            } else {
                if (!token.empty()) {
                    tokens.push_back(token);
                    token.clear();
                }
                in_string = true;
                token += c;
            }
            continue;
        }

        // Handle characters inside a string without further splitting
        if (in_string) {
            token += c;
            continue;
        }

        // Split on whitespace or punctuation, but don't split on underscore
        if (std::isspace(static_cast<unsigned char>(c)) || (std::ispunct(static_cast<unsigned char>(c)) && c != '_')) {
            if (!token.empty()) {
                tokens.push_back(token);
                token.clear();
            }
            if (std::ispunct(static_cast<unsigned char>(c)) && c != '_') {
                tokens.push_back(std::string(1, c));
            }
        } else {
            token += c;
        }
    }

    // Add any remaining token
    if (!token.empty()) {
        tokens.push_back(token);
    }

    return tokens;
}



// Primitive types that should not be converted to const Type&
static std::unordered_set<std::string> primitiveTypes = {"int", "double", "bool"};

// Recursive function to parse types with nesting
std::string parseType(const std::vector<std::string>& tokens, int& pos, const std::unordered_set<std::string>& conceptNames, bool declaring, bool& isConstructorCall, int start_pos) {
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
            std::string nestedType = parseType(tokens, pos, conceptNames, false, isConstructorCall, start_pos);

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
            bool followedByParenthesis = lookahead < tokens.size() && (tokens[lookahead] == "(" || tokens[lookahead]==".");

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
            std::string nestedType = parseType(tokens, pos, conceptNames, false, isConstructorCall, start_pos);
            if (pos >= tokens.size() || tokens[pos] != "]") {
                throw std::runtime_error("Expected ']' after '['");
            }
            pos++; // Move past ']'
            type += "[" + nestedType + "]";
        }
        else if (current == ",") {
            //pos++; // Move past ','
            //type += ", ";
            break;
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
            // if (current == "string") 
            //     current = "std::string";
        
            // If declaring and the type is a function argument, append '&' unless it's a primitive type
            if (declaring && !isConstructorCall) {
                if (primitiveTypes.find(type) == primitiveTypes.end()) {
                    // Not a primitive type, convert to const Type&
                    //type = "const " + type + "&";
                    type = type + "&";
                }
            }
            
            if(current==".") {
                isConstructorCall = false;
                // we are just after a templated type, so do something according to the next token
                if(pos<tokens.size()-1 && tokens[pos+1]=="new") {
                    pos+=2; // also skip "new"
                    continue; // just continue and take the arguments
                }
                pos++;
                type += "(); "+tokens[start_pos]+"->";
                return type;
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

std::vector<std::string> transformTokens(const std::vector<std::string>& tokens, bool injectExtras, std::vector<std::string>& preample, const std::string &transpilation_depth, const std::string &directory) {
    std::vector<std::string> newTokens;
    if(injectExtras)
        newTokens.emplace_back("#include<atomic>\n#include <ranges>\n#include <iostream>\n#include <vector>\n#include <memory>\n#include <string>\n#define print(message) std::cout<<(message)<<std::endl\n");
    std::string fnName("");
    bool declaring = false;
    bool inConcept = false;
    std::unordered_set<std::string> argNames;
    std::unordered_set<std::string> conceptNames;
    std::unordered_set<std::string> namespaces;
    namespaces.insert("cimple");
    if(injectExtras)
        newTokens.emplace_back("\n#define string(message) std::to_string(message)\n");
    
    if(injectExtras)
    newTokens.emplace_back(
        "\n#include <stdexcept>\n"
        "\n"
        "template <typename T>\n"
        "class SafeSharedPtr {\n"
        "public:\n"
        "    SafeSharedPtr() = default;\n"
        "    explicit SafeSharedPtr(T* ptr) : ptr_(std::shared_ptr<T>(ptr)) {}\n"
        "    explicit SafeSharedPtr(const std::shared_ptr<T>& ptr) : ptr_(ptr) {}\n"
        "    explicit SafeSharedPtr(std::shared_ptr<T>&& ptr) : ptr_(std::move(ptr)) {}\n"
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
        "};\n\n"
        "template <typename T, typename... Args>\n"
        "SafeSharedPtr<T> make_safe_shared(Args&&... args) {\n"
        "    return SafeSharedPtr<T>(std::make_shared<T>(std::forward<Args>(args)...));\n"
        "}\n"
        "\n"
    );


    if(injectExtras)
    newTokens.emplace_back(
        "template <typename Iterable>\n"
        "class LockedIterable {\n"
        "private:\n"
        "    Iterable& m_iterable;\n"
        "public:\n"
        "    Iterable& get() {return m_iterable;}\n"
        "    LockedIterable(Iterable& iterable) : m_iterable(iterable) {m_iterable->lock();}\n"
        "    ~LockedIterable() {m_iterable->unlock();}\n"
        "    LockedIterable(const LockedIterable&) = delete;\n"
        "    LockedIterable& operator=(const LockedIterable&) = delete;\n"
        "    LockedIterable(LockedIterable&& other) noexcept : m_iterable(other.m_iterable) {other.m_iterable = nullptr;}\n"
        "    LockedIterable& operator=(LockedIterable&& other) noexcept {\n"
        "        if (this != &other) {m_iterable->unlock();m_iterable = other.m_iterable;other.m_iterable = nullptr;}\n"
        "        return *this;\n"
        "    }\n"
        "    auto begin() const { return m_iterable->begin(); }\n"
        "    auto end() const { return m_iterable->end(); }\n"
        "};\n\n"
    );

    if(injectExtras)
    newTokens.emplace_back(
        "template <typename T>\n"
        "class SafeVector {\n"
        "private:\n"
        "    std::vector<T> data;\n"
        "    std::atomic<int> itercount;\n"
        "\n"
        "public:\n"
        "    SafeVector() = default;\n"
        "    SafeVector(int size) : data(size), itercount(0) {}\n"
        "    SafeVector(std::initializer_list<T> init) : data(init), itercount(0) {}\n"
        "    SafeVector(const SafeVector& other) = delete;\n"
        "    SafeVector(SafeVector<T>&& other) : data(std::move(other.data)), itercount(0) {if(other.itercount) throw std::out_of_range(\"Cannot return a vector from within a loop.\");}\n"
        "    operator auto() const {return data.begin();} \n"
        "    auto lock() { ++itercount; }\n"
        "    auto unlock() { --itercount; }\n"
        "    auto begin() const { return data.begin(); }\n"
        "    auto end() const { return data.end(); }\n"
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
        "        if (itercount) throw std::out_of_range(\"Cannot pop from an iterating vector.\");"
        "        data.pop_back();\n"
        "    }\n"
        "    void reserve(size_t size) { data.reserve(size); }\n"
        "    void push(const T& value) { if (itercount) throw std::out_of_range(\"Cannot push to an iterating vector.\"); data.push_back(value); }\n"
        "    void clear() { if (itercount) throw std::out_of_range(\"Cannot clear an iterating vector.\"); data.clear(); }\n"
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
            std::cerr << "Preprocessor directives are not allowed." << std::endl;
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
        if(tokens[i]=="begin") {
            std::cerr << "`begin` is not allowed as it is unsage and thus automatically applied when safeguards can be obtained" << std::endl;
            exit(1);
        }
        if(tokens[i]=="end") {
            std::cerr << "`end` is not allowed as it is unsage and thus automatically applied when safeguards can be obtained." << std::endl;
            exit(1);
        }
        if(tokens[i]=="new") {
            std::cerr << "`new` is not allowed unless in the pattern handler.new(constuctor arguments)." << std::endl;
            exit(1);
        }
        if(tokens[i]=="-" && i<tokens.size()-1 && tokens[i+1]==">") {
            std::cerr << "`->` is not allowed, as it is automatically inferred. Use `.` instead." << std::endl;
            exit(1);
        }
        if(tokens[i]==":") {
            std::cerr << "`:` is not a valid syntax. Use `in` instead if you are in a for loop." << std::endl;
            exit(1);
        }
        if(tokens[i]=="in") {
            newTokens.emplace_back(":");
            if(i<tokens.size()-1 && tokens[i+1]=="zip")
                continue; // TODO: fix zip
            int depth = 1;
            i++;
            newTokens.emplace_back("LockedIterable");
            newTokens.emplace_back("(");
            while(i<tokens.size()) {
                newTokens.emplace_back(tokens[i]);
                if(tokens[i]=="(")
                    depth++;
                if(tokens[i]==")")
                    depth--;
                if(depth==0)
                    break;
                i++;
            }
            newTokens.emplace_back(")");
            continue;
        }
        if(tokens[i]=="zip") {
            newTokens.emplace_back("std::views::zip");
            continue;
        }
        if(namespaces.find(tokens[i])!=namespaces.end() && tokens[i]!="cimple") {
            newTokens.emplace_back("cimple_"+tokens[i]);
            continue;
        }
        if(tokens[i]=="." && newTokens[newTokens.size()-1][newTokens[newTokens.size()-1].size()-1]=='>') {
            // we are just after a templated type, so do something according to the next token
            if(i<tokens.size()-1 && tokens[i+1]=="new") {
                i++; // also skip "new"
                continue; // just continue and take the arguments
            }
            newTokens.push_back("(");
            newTokens.push_back("(");
            newTokens.push_back("->");
            continue;
        }

        if(tokens[i]=="." && i && namespaces.find(tokens[i-1])!=namespaces.end() ) {
            newTokens.emplace_back(":");
            newTokens.emplace_back(":");
            continue;
        }
        if(tokens[i]=="." && (i==0 || i>=tokens.size()-2 || !isNumber(tokens[i-1]) || !isNumber(tokens[i+1]))) {
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
        if(i<tokens.size()-9 && tokens[i]=="cimple" && tokens[i+1]=="." && tokens[i+2]=="unsafe" && tokens[i+3]=="." && tokens[i+4]=="include" && tokens[i+5]=="(") {
            if(tokens[i+6][0]=='"')
                preample.emplace_back("#include "+tokens[i+6]+"\n");
            else
                preample.emplace_back("#include <"+tokens[i+6]+">\n");
            if(tokens[i+7]!=")" || tokens[i+8]!=";")
                throw std::runtime_error("Invalid unsafe include syntax.");
            i += 8;
            continue;
        }
        if(i<tokens.size()-7 && tokens[i]=="cimple" && tokens[i+1]=="." && tokens[i+2]=="unsafe" && tokens[i+3]=="." && tokens[i+4]=="inline" && tokens[i+5]=="(") {
            int depth = 1;
            int pos = i+6;
            while(pos<tokens.size()) {
                if(tokens[pos]=="(")
                    depth++;
                if(tokens[pos]==")")
                    depth--;
                if(depth==0)
                    break;
                if(tokens[pos]=="#")
                    throw std::runtime_error("For added safety, you cannot also not use preprocessor directives when inlining.");
                newTokens.emplace_back(tokens[pos]);
                pos++;
            }
            i = pos;
            continue;
        }
        if(tokens[i]=="var") {
            if(declaring) {
                std::cerr << "Explicit types are always expected as function arguments." << std::endl;
                exit(1);
            }
            if(i<tokens.size()-8 && tokens[i+2]=="=" && tokens[i+3]=="cimple" && tokens[i+4]==".") {
                if(tokens[i+5]=="import" &&  tokens[i+6]=="(")  {
                    newTokens.emplace_back("\nnamespace");
                    newTokens.emplace_back("cimple_"+tokens[i+1]);
                    newTokens.emplace_back("{");
                    std::cout << transpilation_depth <<  "→ " << tokens[i+7].substr(1, tokens[i+7].find_last_of('\"')-1) << ".cm" << std::endl;
                    //newTokens.emplace_back("#include "+tokens[i+7].substr(0, tokens[i+7].find_last_of('\"')) + ".cpp\"\n");
                    std::ifstream infile(tokens[i + 7].substr(1, tokens[i + 7].find_last_of('\"') - 1) + ".cm");
                    if (!infile.is_open()) {
                        std::string filename = directory + "/" + tokens[i + 7].substr(1, tokens[i + 7].find_last_of('\"') - 1) + ".cm";
                        infile.open(filename);
                        if (!infile.is_open()) 
                            throw std::runtime_error("Could not open file: " + filename);
                    }
                    std::stringstream buffer;
                    buffer << infile.rdbuf();
                    infile.close();
                    std::vector<std::string> fileTokens = tokenize(buffer.str());
                    std::string newDirectory = directory+tokens[i+7].substr(1, tokens[i+7].find_last_of('/'));
                    fileTokens = transformTokens(fileTokens, false, preample, transpilation_depth+"  ", newDirectory);
                    newTokens.insert(newTokens.end(), fileTokens.begin(), fileTokens.end());
                    namespaces.insert(tokens[i+1]);
                    i = i+9;
                    newTokens.emplace_back("}");
                    newTokens.emplace_back("\n");
                }
                else if(i<tokens.size()-9 && tokens[i+5]=="unsafe" && tokens[i+6]=="." && tokens[i+7]=="inline" && tokens[i+8]=="(") {
                    newTokens.emplace_back("auto");
                    newTokens.emplace_back(tokens[i+1]);
                    newTokens.emplace_back("=");
                    int depth = 1;
                    int pos = i+9;
                    while(pos<tokens.size()) {
                        if(tokens[pos]=="(")
                            depth++;
                        if(tokens[pos]==")")
                            depth--;
                        if(depth==0)
                            break;
                        if(tokens[pos]=="#")
                            throw std::runtime_error("For added safety, you cannot also not use preprocessor directives when inlining.");
                        newTokens.emplace_back(tokens[pos]);
                        pos++;
                    }
                    i = pos;
                }
                else
                    throw std::runtime_error("Invalid instruction for cimple.");
                continue;
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
                std::string parsedType = parseType(tokens, posCopy, conceptNames, declaring, isConstructorCall, i-2);
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
        //if(tokens[i]=="string") {
        //    newTokens.emplace_back("std::string");
        //    continue;
        //}

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
    std::cout << "  Building: " << filename << std::endl;
    std::vector<std::string> preample;
    tokens = transformTokens(tokens, true, preample, "    ", filename.substr(0, filename.find_last_of('/')));
    tokens.insert(tokens.begin(), preample.begin(), preample.end());

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
    std::cout << "  Compiling: " << output_filename << std::endl;

    std::string executable_name = filename.substr(0, filename.find_last_of('.'));
    std::string compile_command = "g++ " + output_filename + " -o "+executable_name+" -O2 -std=c++23";
    if (std::system(compile_command.c_str()) != 0) 
        std::cerr << "Failed to compile the generated code." << std::endl;
    std::string run_command = "./" + executable_name;
    std::cout << "  Running: " << run_command << std::endl;
    std::cout << "--------------------------------------------" << std::endl;
    if (std::system(run_command.c_str()) != 0) 
        std::cerr << "Failed to run the generated code." << std::endl;
    //else
    //    std::cout << "Execution finished" << std::endl;

}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <source.cm>" << std::endl;
        return 1;
    }
    std::cout << "--------------- Cimple v0.1 ----------------" << std::endl;
    processFile(argv[1]);
    return 0;
}
