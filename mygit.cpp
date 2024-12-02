#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <ctime>
#include <vector>
#include <iomanip>
#include <openssl/sha.h>
#include <stdexcept>
using namespace std;
namespace fs = std::filesystem;

// FUNCTION TO CALCULATE SHA1
std::string sha1(const std::string &data)
{
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char *>(data.c_str()), data.size(), hash);
    ostringstream result;
    for (int i = 0; i < SHA_DIGEST_LENGTH; ++i)
    {
        result << hex << setw(2) << setfill('0') << (int)hash[i];
    }
    return result.str();
}

// REPOSITORY INITIALIAZED
void init()
{
    fs::path repoPath = ".mygit";
    if (fs::exists(repoPath))
    {
        cerr << "Error: Repository already initialized." << endl;
        return;
    }
    try
    {
        fs::create_directory(repoPath);
        fs::create_directory(repoPath / "objects");
        fs::create_directory(repoPath / "refs");
        fs::create_directory(repoPath / "refs/heads");
        ofstream headFile(repoPath / "HEAD");
        headFile << "ref: refs/heads/main";
        headFile.close();
        std::ofstream mainBranch(repoPath / "refs/heads/main");
        mainBranch.close();
        cout << "Initialized empty repository in .mygit" << endl;
    }
    catch (const fs::filesystem_error &e)
    {
        cerr << "Filesystem error: " << e.what() << endl;
    }
}

// FILE HASHING AND WRITE IT TO THE OBJECTS DIRECTORY
string hashObject(const string &filePath, bool write)
{
    std::ifstream file(filePath, std::ios::binary);
    if (!file)
    {
        cerr << "Error: File not found." << endl;
        return "";
    }
    std::ostringstream fileContents;
    fileContents << file.rdbuf();
    string content = fileContents.str();
    string hash = sha1(content);

    if (write)
    {
        try
        {
            fs::path objectPath = ".mygit/objects/" + hash.substr(0, 2);
            if (!fs::exists(objectPath))
            {
                fs::create_directory(objectPath);
            }
            ofstream outFile(objectPath / hash.substr(2), std::ios::binary);
            if (!outFile)
            {
                throw std::ios_base::failure("Error: Unable to write object file.");
            }
            outFile << content;
            outFile.close();
            cout << "Object successfully written with SHA-1: " << hash << endl; // Added feedback
        }
        catch (const fs::filesystem_error &e)
        {
            cerr << "Filesystem error: " << e.what() << endl;
        }
        catch (const std::exception &e)
        {
            cerr << "Error: " << e.what() << endl;
        }
    }
    else
    {
        cout << "SHA-1: " << hash << endl;
    }

    return hash;
}

// Read the contents of a file stored as an object
void catFile(const string &hash, const std::string &flag)
{
    fs::path objectPath = ".mygit/objects/" + hash.substr(0, 2) + "/" + hash.substr(2);
    if (!fs::exists(objectPath))
    {
        cerr << "Error: Object not found." << endl;
        return;
    }
    ifstream objectFile(objectPath, std::ios::binary);
    ostringstream content;
    content << objectFile.rdbuf();
    if (flag == "-p")
    {
        cout << content.str() << endl;
    }
    else if (flag == "-s")
    {
        cout << "Size: " << content.str().size() << " bytes" << endl;
    }
    else if (flag == "-t")
    {
        cout << "Type: blob" << endl;
    }
    else
    {
        cerr << "Error: Invalid flag." << endl;
    }
}

// WRITING DIRECTORY AS TREE OBJECT
string writeTree()
{
    try
    {
        ostringstream tree;
        for (const auto &entry : fs::recursive_directory_iterator(fs::current_path()))
        {
            if (entry.path().string().find(".mygit") != std::string::npos)
                continue;

            // Determine type and set appropriate mode
            string type = entry.is_directory() ? "tree" : "blob";
            string mode = entry.is_directory() ? "040000" : "100644";

            // Hash the content of files
            string contentHash;
            if (entry.is_regular_file())
            {
                ifstream file(entry.path(), std::ios::binary);
                if (!file)
                {
                    cerr << "Error: Unable to read file " << entry.path() << endl;
                    continue;
                }
                ostringstream fileContents;
                fileContents << file.rdbuf();
                contentHash = sha1(fileContents.str());
            }
            else
            {
                contentHash = sha1(entry.path().string());
            }
            tree << mode << " " << type << " " << contentHash << " " << entry.path().filename().string() << "\n";
        }

        // HASH FOR TREE CONTENT
        string treeContent = tree.str();
        string treeHash = sha1(treeContent);

        fs::path objectPath = ".mygit/objects/" + treeHash.substr(0, 2);
        if (!fs::exists(objectPath))
        {
            fs::create_directory(objectPath);
        }
        ofstream outFile(objectPath / treeHash.substr(2), std::ios::binary);
        if (!outFile)
        {
            throw std::ios_base::failure("Error: Unable to write tree object file.");
        }
        outFile << treeContent;
        outFile.close();

        // Print the tree hash as output
        cout << "Tree SHA-1: " << treeHash << endl;
        return treeHash;
    }
    catch (const fs::filesystem_error &e)
    {
        cerr << "Filesystem error: " << e.what() << endl;
    }
    catch (const std::exception &e)
    {
        cerr << "Error: " << e.what() << endl;
    }

    return "";
}
// LIST THE CONTENTS OF TREE OBJECT
void lsTree(const std::string &treeHash, bool nameOnly = false)
{
    fs::path objectPath = ".mygit/objects/" + treeHash.substr(0, 2) + "/" + treeHash.substr(2);

    if (!fs::exists(objectPath))
    {
        cerr << "Error: Tree object not found." << endl;
        return;
    }
    ifstream treeFile(objectPath, std::ios::binary);
    if (!treeFile)
    {
        cerr << "Error: Unable to open tree object file." << endl;
        return;
    }
    string line;
    while (std::getline(treeFile, line))
    {
        istringstream iss(line);
        string mode, type, hash, name;

        if (!(iss >> mode >> type >> hash >> name))
        {
            cerr << "Error: Malformed tree object entry." << endl;
            return;
        }

        if (nameOnly)
        {
            cout << name << std::endl;
        }
        else
        {
            cout << mode << " " << type << " " << hash << " " << name << endl;
        }
    }
}
// ADDING FILES TO STAGING AREA
void add(const std::vector<std::string> &files)
{
    ofstream indexFile(".mygit/index", ios::app);
    if (!indexFile)
    {
        cerr << "Error: Unable to open index file for writing." << endl;
        return;
    }

    bool anyFileAdded = false;

    for (const auto &file : files)
    {
        if (file == ".")
        {
            // If the argument is ".", stage all files in the current directory
            for (const auto &entry : fs::recursive_directory_iterator(fs::current_path()))
            {
                if (fs::is_regular_file(entry))
                {
                    string filePath = entry.path().string();
                    ifstream inFile(filePath, ios::binary);
                    if (!inFile)
                    {
                        cerr << "Error: File " << filePath << " not found." << endl;
                        continue;
                    }

                    ostringstream fileContents;
                    fileContents << inFile.rdbuf();
                    string hash = sha1(fileContents.str());
                    indexFile << filePath << " " << hash << "\n";
                    cout << "Added " << filePath << " to staging area." << endl;
                    anyFileAdded = true;
                }
            }
        }
        else
        {
            // If the argument is a specific file, stage that file
            ifstream inFile(file, ios::binary);
            if (!inFile)
            {
                cerr << "Error: File " << file << " not found." << endl;
                continue;
            }
            ostringstream fileContents;
            fileContents << inFile.rdbuf();
            string hash = sha1(fileContents.str());
            indexFile << file << " " << hash << "\n";
            cout << "Added " << file << " to staging area." << endl;
            anyFileAdded = true;
        }
    }

    if (!anyFileAdded)
    {
        cout << "No files were added to the staging area." << endl;
    }
}
// COMMIT STAGED CHANGES
void commit(const std::string &message, const std::string &committer = "aos aos  <AOS4@example.com>")
{
    ifstream indexFile(".mygit/index");
    if (!indexFile)
    {
        cerr << "Error: Nothing to commit." << endl;
        return;
    }
    ostringstream commitContent;
    string treeHash = writeTree();
    if (treeHash.empty())
    {
        cerr << "Error: Failed to write tree object." << endl;
        return;
    }
    commitContent << "tree: " << treeHash << "\n";
    ifstream headFile(".mygit/HEAD");
    string headRef, parentHash;
    getline(headFile, headRef);
    if (headRef.find("ref: ") == 0)
    {
        ifstream refFile(".mygit/" + headRef.substr(5));
        if (refFile)
        {
            getline(refFile, parentHash);
            if (!parentHash.empty())
            {
                commitContent << "parent: " << parentHash << "\n";
            }
        }
    }
    commitContent << "message: " << message << "\n";
    std::time_t now = std::time(nullptr);
    commitContent << "timestamp: " << std::asctime(std::localtime(&now));
    commitContent << "committer: " << committer << "\n";
    std::string commitHash = sha1(commitContent.str());
    try
    {
        fs::path objectPath = ".mygit/objects/" + commitHash.substr(0, 2);
        if (!fs::exists(objectPath))
        {
            fs::create_directory(objectPath);
        }
        ofstream outFile(objectPath / commitHash.substr(2), std::ios::binary);
        if (!outFile)
        {
            throw std::ios_base::failure("Error: Unable to write commit object file.");
        }
        outFile << commitContent.str();
        outFile.close();
        ofstream refFile(".mygit/" + headRef.substr(5), std::ios::trunc);
        if (!refFile)
        {
            throw std::ios_base::failure("Error: Unable to update reference file.");
        }
        refFile << commitHash;
        refFile.close();

        cout << "Commit SHA-1: " << commitHash << endl;
    }
    catch (const fs::filesystem_error &e)
    {
        cerr << "Filesystem error: " << e.what() << endl;
    }
    catch (const std::exception &e)
    {
        cerr << "Error: " << e.what() << endl;
    }
}

// Display commit history
void logHistory()
{
    ifstream headFile(".mygit/HEAD");
    if (!headFile)
    {
        cerr << "Error: No commits found. Repository may not be initialized or empty." << endl;
        return;
    }
    string headRef;
    getline(headFile, headRef);
    if (headRef.find("ref: ") != 0)
    {
        cerr << "Error: HEAD file is malformed." << endl;
        return;
    }
    ifstream refFile(".mygit/" + headRef.substr(5));
    if (!refFile)
    {
        cerr << "Error: No commits found. Repository may not be initialized or empty." << endl;
        return;
    }
    string currentCommit;
    getline(refFile, currentCommit);
    if (currentCommit.empty())
    {
        cerr << "Error: No commits found." << endl;
        return;
    }

    while (!currentCommit.empty())
    {
        fs::path commitPath = ".mygit/objects/" + currentCommit.substr(0, 2) + "/" + currentCommit.substr(2);
        if (!fs::exists(commitPath))
        {
            cerr << "Error: Commit object not found: " << currentCommit << endl;
            return;
        }

        ifstream commitFile(commitPath, std::ios::binary);
        if (!commitFile)
        {
            cerr << "Error: Unable to open commit file: " << currentCommit << endl;
            return;
        }

        ostringstream commitContent;
        commitContent << commitFile.rdbuf();
        istringstream commitStream(commitContent.str());

        string line, parent, message, timestamp, committer;
        while (getline(commitStream, line))
        {
            if (line.find("parent: ") == 0)
            {
                parent = line.substr(8);
            }
            else if (line.find("message: ") == 0)
            {
                message = line.substr(9);
            }
            else if (line.find("timestamp: ") == 0)
            {
                timestamp = line.substr(11);
            }
            else if (line.find("committer: ") == 0)
            {
                committer = line.substr(11);
            }
        }

        cout << "Commit: " << currentCommit << endl;
        if (!parent.empty())
        {
            cout << "Parent: " << parent << endl;
        }
        if (!committer.empty())
        {
            cout << "Committer: " << committer << endl;
        }
        cout << "Date: " << timestamp << endl;
        cout << "Message: " << message << endl;
        cout << endl;

        currentCommit = parent;
    }
}
void checkout(const std::string &commit_sha)
{
    string commit_path = ".mygit/objects/" + commit_sha.substr(0, 2) + "/" + commit_sha.substr(2);
    cout << "Looking for commit at path: " << commit_path << endl;

    if (!fs::exists(commit_path))
    {
        cerr << "Error: Commit object not found at " << commit_path << endl;
        return;
    }

    ifstream commit_file(commit_path);
    if (!commit_file)
    {
        cerr << "Error: Unable to read the commit object at " << commit_path << endl;
        return;
    }
    cout << "Commit object successfully opened." << endl;
    string line, tree_sha;
    while (getline(commit_file, line))
    {
        cout << "Reading commit line: " << line << endl;
        if (line.find("tree: ") == 0)
        {
            tree_sha = line.substr(6);
            cout << "Tree SHA found: " << tree_sha << endl;
            break;
        }
    }
    commit_file.close();

    if (tree_sha.empty())
    {
        cerr << "Error: Tree SHA not found in commit object." << endl;
        return;
    }

    string tree_path = ".mygit/objects/" + tree_sha.substr(0, 2) + "/" + tree_sha.substr(2);
    cout << "Looking for tree at path: " << tree_path << endl;

    if (!fs::exists(tree_path))
    {
        cerr << "Error: Tree object not found at " << tree_path << endl;
        return;
    }
    ifstream tree_file(tree_path);
    if (!tree_file)
    {
        cerr << "Error: Unable to read the tree object at " << tree_path << endl;
        return;
    }
    cout << "Tree object successfully opened." << endl;

    for (const auto &entry : fs::directory_iterator("."))
    {
        if (entry.path().filename() == ".mygit")
        {
            continue;
        }
        fs::remove_all(entry.path());
        cout << "Removed: " << entry.path() << endl;
    }
    string tree_line;
    while (getline(tree_file, tree_line))
    {
        istringstream iss(tree_line);
        string mode, type, blob_sha, file_path;
        if (!(iss >> mode >> type >> blob_sha >> file_path))
        {
            cerr << "Error: Malformed tree object entry: " << tree_line << endl;
            return;
        }
        cout << "Restoring: " << file_path << " with blob SHA: " << blob_sha << endl;
        string blob_path = ".mygit/objects/" + blob_sha.substr(0, 2) + "/" + blob_sha.substr(2);
        if (!fs::exists(blob_path))
        {
            cerr << "Error: Blob object not found for file " << file_path << endl;
            continue;
        }

        fs::create_directories(fs::path(file_path).parent_path());

        ifstream blob_file(blob_path);
        if (!blob_file)
        {
            cerr << "Error: Unable to read blob object at " << blob_path << endl;
            continue;
        }
        ofstream restored_file(file_path);
        restored_file << blob_file.rdbuf();
        blob_file.close();
        restored_file.close();
        cout << "Restored: " << file_path << endl;
    }

    tree_file.close();
    cout << "Checked out commit " << commit_sha << endl;
}
int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        cerr << "Error: No command provided." << endl;
        return 1;
    }
    string command = argv[1];
    try
    {
        if (command == "init")
        {
            init();
        }
        else if (command == "hash-object")
        {
            bool write = (argc >= 3 && std::string(argv[2]) == "-w");
            hashObject(argv[argc - 1], write);
        }
        else if (command == "cat-file")
        {
            if (argc != 4)
            {
                cerr << "Usage: ./mygit cat-file <flag> <file_sha>" << endl;
                return 1;
            }
            catFile(argv[3], argv[2]);
        }
        else if (command == "write-tree")
        {
            writeTree();
        }
        else if (command == "ls-tree")
        {
            bool nameOnly = (argc == 4 && std::string(argv[3]) == "--name-only");
            lsTree(argv[2], nameOnly);
        }
        else if (command == "add")
        {
            add(std::vector<std::string>(argv + 2, argv + argc));
            cout << "added to staging area";
        }
        else if (command == "commit")
        {
            string message = (argc == 4) ? argv[3] : "Default commit message";
            commit(message);
        }
        else if (command == "log")
        {
            logHistory();
        }
        else if (command == "checkout")
        {
            if (argc != 3)
            {
                cerr << "Usage: ./mygit checkout <commit_sha>" << endl;
                return 1;
            }
            checkout(argv[2]);
        }
        else
        {
            cerr << "Error: Unknown command." << endl;
        }
    }
    catch (const std::exception &e)
    {
        cerr << "Error: " << e.what() << endl;
    }
    return 0;
}
