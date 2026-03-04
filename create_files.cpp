#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

using namespace std;
namespace fs = std::filesystem;

void write_file(
  const fs::path& p, const string& content, chrono::system_clock::time_point tp
) {
  fs::create_directories(p.parent_path());
  ofstream out(p, ios::binary);
  out << content;
  out.close();

  auto sctp = chrono::time_point_cast<fs::file_time_type::duration>(tp);
  fs::last_write_time(p, fs::file_time_type(sctp.time_since_epoch()));
}

void create_files(const fs::path& root) {
  fs::remove_all(root);

  using namespace chrono;
  auto base = system_clock::now();

  write_file(root / "folder/file1.txt", "content", base);
  write_file(root / "folder/file2.txt", "content", base + seconds(1));
  write_file(root / "folder2/subfolder/file3.txt", "content", base + hours(1));
  write_file(root / "folder3/data.bin", "copy", base + seconds(2));
  write_file(root / "folder2/subfolder/data.bin", "copy", base + seconds(3));
  write_file(root / "folder/subfolder/data.bin", "copy", base + hours(2));

  string json = R"({
    "name": "test config",
    "version": "1.0.0"
  })";

  write_file(root / "folder/config1.json", json, base + hours(3));
  write_file(root / "folder/subfolder/config2.json", json, base + hours(4));

  string csv =
    "id,name,value\n"
    "1,alice,100\n"
    "2,bob,200\n"
    "3,charlie,300\n";

  write_file(root / "folder2/data1.csv", csv, base + minutes(10));
  write_file(root / "folder3/data2.csv", csv, base + minutes(11));

  string content;

  for (int i = 1; i <= 100; ++i) {
    content += "Line " + to_string(i) + "\n";
  }

  write_file(root / "folder/largefile1.txt", content, base + seconds(10));
  write_file(root / "folder3/foo/largefile2.txt", content, base + seconds(11));
  write_file(root / "folder3/foo/largefile3.txt", content, base + seconds(11));
}

int main() {
  create_files("test_env");
  return 0;
}
