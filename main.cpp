#include <cstdint>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <openssl/evp.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;
using recursive_dir_iterator = filesystem::recursive_directory_iterator;

struct FileMetadata {
  filesystem::path path;
  time_t creation_time;
  string formatted_time;
};

string sha256_file(const filesystem::path &path) {
  ifstream file(path, ios::binary);

  if (!file.is_open())
    throw runtime_error("Cannot open file: " + path.string());

  unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> ctx(
    EVP_MD_CTX_new(), &EVP_MD_CTX_free
  );

  if (!ctx) throw runtime_error("Failed to create EVP context");

  if (!EVP_DigestInit_ex(ctx.get(), EVP_sha256(), nullptr))
    throw runtime_error("Failed to initialize digest");

  char buffer[65536];

  while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) {
    streamsize bytes = file.gcount();

    if (!EVP_DigestUpdate(ctx.get(), buffer, bytes))
      throw runtime_error("Failed to update digest");
  }

  unsigned char digest[EVP_MAX_MD_SIZE];
  unsigned int digest_len = 0;

  if (!EVP_DigestFinal_ex(ctx.get(), digest, &digest_len))
    throw runtime_error("Failed to finalize digest");

  static const char hex[] = "0123456789abcdef";
  string out;
  out.reserve(digest_len * 2);

  for (unsigned int i = 0; i < digest_len; ++i) {
    out.push_back(hex[digest[i] >> 4]);
    out.push_back(hex[digest[i] & 0x0F]);
  }

  return out;
}

string format_time(time_t tt) {
  stringstream ss;
  ss << put_time(localtime(&tt), "%Y-%m-%d %H:%M:%S");
  return ss.str();
}

bool should_skip_path(
  const filesystem::path &path, const unordered_set<string> &exception_dirs
) {
  for (const auto &part : path) {
    string dir_name = part.string();
    // Skip current dir "." and parent dir ".."
    if (dir_name != "." && dir_name != ".." && exception_dirs.count(dir_name))
      return true;
  }
  
  return false;
}

unordered_map<uintmax_t, vector<filesystem::path>> group_files_by_size(
  const filesystem::path &base_dir,
  const unordered_set<string> &exception_dirs
) {
  unordered_map<uintmax_t, vector<filesystem::path>> mapped_files;
  auto opts = filesystem::directory_options::skip_permission_denied;

  for (const auto &entry : recursive_dir_iterator(base_dir, opts)) {
    try {
      if (!entry.is_regular_file()) continue;

      if (should_skip_path(entry.path(), exception_dirs)) continue;

      mapped_files[entry.file_size()].push_back(entry.path());
    } catch (const filesystem::filesystem_error &e) {
      cerr << "Cannot access " << entry.path() << ": " << e.what() << "\n";
    }
  }

  return mapped_files;
}

void hash_files(
  const vector<filesystem::path> &files,
  unordered_map<string,
  vector<filesystem::path>> &hashed_files
) {
  if (files.size() < 2) return;

  for (const auto &path : files) {
    try {
      string hash = sha256_file(path);
      hashed_files[hash].push_back(path);
    } catch (const exception &e) {
      cerr << "Cannot hash file " << path << ": " << e.what() << "\n";
    }
  }
}

int delete_copies(
  const vector<FileMetadata> &duplicates, const filesystem::path &original
) {
  int deleted_count = 0;

  for (const auto &file : duplicates) {
    if (file.path == original) continue;

    try {
      filesystem::remove(file.path);
      deleted_count++;
    } catch (const filesystem::filesystem_error &e) {
      cerr << "ERROR: Cannot delete " << file.path << ": " << e.what() << "\n";
    }
  }

  return deleted_count;
}

filesystem::path find_original(const vector<FileMetadata> &files_metadata) {
  filesystem::path original = files_metadata[0].path;
  time_t oldest_time = files_metadata[0].creation_time;

  for (size_t i = 1; i < files_metadata.size(); ++i) {
    if (files_metadata[i].creation_time < oldest_time) {
      oldest_time = files_metadata[i].creation_time;
      original = files_metadata[i].path;
    }
  }

  return original;
}

void process_duplicate_group(
  const string &hash,
  const vector<filesystem::path> &files,
  int &total_groups,
  int &total_deleted
) {
  if (files.size() < 2) return;

  // Create metadata objects with timestamps
  vector<FileMetadata> files_metadata;

  for (const auto &path : files) {
    auto fs_time = filesystem::last_write_time(path);
    auto duration = fs_time.time_since_epoch();
    auto seconds = chrono::duration_cast<chrono::seconds>(duration);
    time_t tt = seconds.count();
    string formatted = format_time(tt);
    files_metadata.push_back({path, tt, formatted});
  }

  // Find original (oldest file)
  filesystem::path original = find_original(files_metadata);

  cout << "SHA256: " << hash << "\n";
  for (const auto &file : files_metadata) {
    string label = (file.path == original) ? " <- ORIGINAL\n" : "\n";
    cout << "    [" << file.formatted_time << "]" << file.path  << label;
  }

  int deleted_count = delete_copies(files_metadata, original);

  cout << "    Deleted: " << deleted_count << "\n\n";

  total_groups++;
  total_deleted += deleted_count;
}

void process_duplicates(
  const unordered_map<string, vector<filesystem::path>> &hashed_files
) {
  int total_groups = 0, total_deleted = 0;

  for (const auto &[hash, files] : hashed_files) {
    if (files.size() < 2) continue;
    process_duplicate_group(hash, files, total_groups, total_deleted);
  }

  cout << "\n===== SUMMARY =====\n";
  if (total_groups > 0) {
    cout << "Groups found: " << total_groups << "\n";
    cout << "Files deleted: " << total_deleted << "\n";
  } else {
    cout << "No duplicates found.\n";
  }
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    cerr << "Usage: " << argv[0] << " <directory> [-d dir1] [-d dir2]  ...\n";
    return 1;
  }

  filesystem::path base_dir(argv[1]);

  if (!filesystem::exists(base_dir)) {
    cerr << "Error: Directory does not exist: " << base_dir << "\n";
    return 1;
  }

  if (!filesystem::is_directory(base_dir)) {
    cerr << "Error: Path is not a directory: " << base_dir << "\n";
    return 1;
  }

  // Collect exception dirs from remaining arguments
  // Always exclude .git and node_modules by default
  unordered_set<string> exception_dirs = {".git", "node_modules"};

  for (int i = 2; i < argc; ++i) {
    string arg = argv[i];
    if (arg.substr(0, 2) == "-d" && arg.length() > 2)
      // -dfoldername format
      exception_dirs.insert(arg.substr(2));
    else if (arg == "-d" && i + 1 < argc)
      // -d foldername format
      exception_dirs.insert(argv[++i]);
  }

  if (exception_dirs.size() > 2) {
    cout << "Exception directories: ";

    for (const auto &dir : exception_dirs) cout << dir << " ";

    cout << "\n";
  }

  unordered_map<uintmax_t, vector<filesystem::path>> mapped_files;
  unordered_map<string, vector<filesystem::path>> hashed_files;

  cout << "Scanning: " << base_dir << "\n\n";

  try {
    mapped_files = group_files_by_size(base_dir, exception_dirs);
  } catch (const filesystem::filesystem_error &e) {
    cerr << "Error: Cannot read directory: " << e.what() << "\n";
    return 1;
  }

  if (mapped_files.empty()) {
    cout << "No files found.\n";
    return 0;
  }

  for (const auto &[size, files] : mapped_files)
    hash_files(files, hashed_files);

  process_duplicates(hashed_files);

  return 0;
}
