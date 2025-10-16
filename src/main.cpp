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
    for(int i=4;i<argc;++i) {
        std::string a = argv[i];
        if(a == "--push") do_push = true;
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

    // Stage
    std::string cmd_cd = "cd /d \"" + repo.string() + "\"";
    // On Linux/macOS, `cd /d` is invalid; use plain cd. We'll detect at runtime by checking environment.
#ifdef _WIN32
    std::string git_add = cmd_cd + " && git add -A";
    std::string git_commit = cmd_cd + " && git commit -m \"" + commit_msg + "\"";
    std::string git_push = cmd_cd + " && git push";
#else
    std::string cmd_cd_unix = "cd \"" + repo.string() + "\"";
    std::string git_add = cmd_cd_unix + " && git add -A";
    std::string git_commit = cmd_cd_unix + " && git commit -m \"" + commit_msg + "\"";
    std::string git_push = cmd_cd_unix + " && git push";
#endif

    int rc = run_command(git_add);
    if(rc != 0) {
        std::cerr << "git add failed or returned non-zero: " << rc << std::endl;
        // continue to commit to possibly create commit for new repo
    }

    rc = run_command(git_commit);
    if(rc != 0) {
        std::cerr << "git commit failed or returned non-zero: " << rc << std::endl;
        // Could be no changes; still not fatal
    }

    if(do_push) {
        rc = run_command(git_push);
        if(rc != 0) {
            std::cerr << "git push failed or returned non-zero: " << rc << std::endl;
            return 5;
        }
    }

    std::cout << "Done." << std::endl;
    return 0;
}
