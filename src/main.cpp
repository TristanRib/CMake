#include <iostream>
#include <filesystem>
#include <string>
#include <vector>
#include <cstdlib>

namespace fs = std::filesystem;

void print_usage()
{
    std::cout << "Usage: repo_uploader <source_path> <repo_path> <commit_message> [--push]\n";
    std::cout << "Copies files from source_path into repo_path, stages, commits and optionally pushes.\n";
}

int run_command(const std::string &cmd)
{
    std::cout << "+ " << cmd << std::endl;
    int rc = std::system(cmd.c_str());
    if(rc == -1) {
        std::cerr << "Failed to run command: " << cmd << std::endl;
        return -1;
    }
    return rc;
}

bool copy_recursive(const fs::path &from, const fs::path &to)
{
    try {
        if(!fs::exists(from)) {
            std::cerr << "Source path does not exist: " << from << std::endl;
            return false;
        }
        if(fs::is_regular_file(from)) {
            fs::create_directories(to.parent_path());
            fs::copy_file(from, to, fs::copy_options::overwrite_existing);
        } else {
            for(auto &p : fs::recursive_directory_iterator(from)) {
                const auto rel = fs::relative(p.path(), from);
                const auto dest = to / rel;
                if(fs::is_directory(p.path())) {
                    fs::create_directories(dest);
                } else if(fs::is_regular_file(p.path())) {
                    fs::create_directories(dest.parent_path());
                    fs::copy_file(p.path(), dest, fs::copy_options::overwrite_existing);
                }
            }
        }
        return true;
    } catch(const std::exception &e) {
        std::cerr << "Copy failed: " << e.what() << std::endl;
        return false;
    }
}

int main(int argc, char **argv)
{
    if(argc < 4) {
        print_usage();
        return 1;
    }

    fs::path source = argv[1];
    fs::path repo = argv[2];
    std::string commit_msg = argv[3];
    bool do_push = false;
    std::string git_token;
    std::string remote_url;
    std::string branch = "main";

    for(int i=4;i<argc;++i) {
        std::string a = argv[i];
        if(a == "--push") do_push = true;
        else if(a == "--git-token" && i+1<argc) { git_token = argv[++i]; }
        else if(a == "--remote-url" && i+1<argc) { remote_url = argv[++i]; }
        else if(a == "--branch" && i+1<argc) { branch = argv[++i]; }
    }

    if(!fs::exists(repo)) {
        std::cerr << "Repo path does not exist: " << repo << std::endl;
        return 2;
    }

    // Copy contents
    fs::path dest = repo / fs::path(".");
    if(!copy_recursive(source, dest)) {
        return 3;
    }

    // Prepare platform-specific cd
#ifdef _WIN32
    std::string cmd_cd = "cd /d \"" + repo.string() + "\"";
#else
    std::string cmd_cd = "cd \"" + repo.string() + "\"";
#endif

    std::string git_add = cmd_cd + " && git add -A";
    std::string git_commit = cmd_cd + " && git commit -m \"" + commit_msg + "\"";

    int rc = run_command(git_add);
    if(rc != 0) {
        std::cerr << "git add failed or returned non-zero: " << rc << std::endl;
    }

    rc = run_command(git_commit);
    if(rc != 0) {
        std::cerr << "git commit failed or returned non-zero: " << rc << std::endl;
    }

    if(do_push) {
        // If git_token and remote_url are provided, temporarily add a remote that includes the token
        bool removed_remote = false;
        std::string temp_remote_name = "temp-token-remote";
        std::string push_cmd;

        if(!git_token.empty() && !remote_url.empty()) {
            // Inject token into remote URL (https://token@github.com/owner/repo.git)
            std::string modified = remote_url;
            // If URL starts with https://, insert token after scheme
            const std::string https = "https://";
            if(modified.rfind(https, 0) == 0) {
                modified = https + git_token + "@" + modified.substr(https.size());
            } else {
                // Fallback: prefix token@
                modified = git_token + "@" + modified;
            }

            std::string git_remote_add = cmd_cd + " && git remote remove " + temp_remote_name + " 2>nul || true && git remote add " + temp_remote_name + " \"" + modified + "\"";
            rc = run_command(git_remote_add);
            if(rc != 0) {
                std::cerr << "Failed to add temporary remote: " << rc << std::endl;
            } else {
                removed_remote = true;
                push_cmd = cmd_cd + " && git push " + temp_remote_name + " " + branch;
            }
        } else {
            // Use existing remote
            push_cmd = cmd_cd + " && git push origin " + branch;
        }

        rc = run_command(push_cmd);
        if(rc != 0) {
            std::cerr << "git push failed or returned non-zero: " << rc << std::endl;
            // try to cleanup remote if it was added
            if(removed_remote) {
                std::string cleanup = cmd_cd + " && git remote remove " + temp_remote_name;
                run_command(cleanup);
            }
            return 5;
        }

        if(removed_remote) {
            std::string cleanup = cmd_cd + " && git remote remove " + temp_remote_name;
            run_command(cleanup);
        }
    }

    std::cout << "Done." << std::endl;
    return 0;
}
