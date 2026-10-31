// Tests for the engine. No Qt and no test library: a failed CHECK prints its line and the program
// ends with a non-zero code. Files are created under a temporary folder that is removed afterwards.
#include "apps/desktopapps.h"
#include "apps/filetypegroups.h"
#include "archive/archiver.h"
#include "contentinfo/contentinfo.h"
#include "directory/directorylister.h"
#include "drives/drives.h"
#include "fileops/fileops.h"
#include "foldericons/foldericons.h"
#include "filetype/filetype.h"
#include "folderview/folderview.h"
#include "format/format.h"
#include "hashing/hashing.h"
#include "navigation/navigationhistory.h"
#include "network/networkshares.h"
#include "pathbar/pathbar.h"
#include "permissions/permissions.h"
#include "places/places.h"
#include "process/process.h"
#include "search/filesearch.h"
#include "properties/properties.h"
#include "recent/recentfiles.h"
#include "shortcuts/shortcutmap.h"
#include "tabs/tabmanager.h"
#include "thumbnails/thumbnails.h"
#include "transfer/transfer.h"
#include "trash/trash.h"
#include "undo/undostack.h"
#include "watcher/folderwatcher.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <future>
#include <string>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>

namespace fs = std::filesystem;

static int failures = 0;

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            std::printf("FAILED line %d: %s\n", __LINE__, #condition);       \
            ++failures;                                                      \
        }                                                                    \
    } while (false)

static void touch(const fs::path& path, const std::string& text = "")
{
    std::ofstream(path) << text;
}

static Entry fileEntry(const std::string& name, std::uint64_t size = 0, bool directory = false)
{
    Entry entry;
    entry.name = name;
    entry.path = "/x/" + name;
    entry.size = size;
    entry.kind = directory ? EntryKind::Directory : EntryKind::File;
    entry.hidden = name[0] == '.';
    detectFileType(entry);
    return entry;
}

static void testNaturalOrder()
{
    CHECK(naturalCompare("file2", "file10") < 0);
    CHECK(naturalCompare("file10", "file2") > 0);
    CHECK(naturalCompare("File", "file") != 0);
    CHECK(naturalCompare("abc", "ABD") < 0);
    CHECK(naturalCompare("a007", "a7") != 0);
    CHECK(naturalCompare("a1b", "a1b") == 0);
    CHECK(naturalCompare("99999999999999999999998", "99999999999999999999999") < 0);
}

static void testSorting()
{
    std::vector<Entry> entries {fileEntry("b.txt", 5), fileEntry("zdir", 0, true), fileEntry("a.txt", 9)};
    sortEntries(entries, {SortKey::Name, true});
    CHECK(entries[0].name == "zdir" && entries[1].name == "a.txt");

    sortEntries(entries, {SortKey::Name, false});
    CHECK(entries[0].name == "zdir" && entries[1].name == "b.txt");   // folders stay first

    sortEntries(entries, {SortKey::Size, true});
    CHECK(entries[1].name == "b.txt" && entries[2].name == "a.txt");
}

static void testFileTypes()
{
    Entry png = fileEntry("Photo.PNG");
    CHECK(png.category == FileCategory::Image && png.typeName == "PNG Image");

    Entry unknown = fileEntry("data.xyz");
    CHECK(unknown.typeName == "XYZ File");

    Entry plain = fileEntry("Makefile");
    CHECK(plain.typeName == "File");

    Entry dotfile = fileEntry(".bashrc");
    CHECK(dotfile.typeName == "File");

    Entry program = fileEntry("runme");
    program.mode = 0755;
    detectFileType(program);
    CHECK(program.category == FileCategory::Program);
}

static void testFormat()
{
    CHECK(Format::size(0) == "0 bytes");
    CHECK(Format::size(1) == "1 byte");
    CHECK(Format::size(1023) == "1023 bytes");
    CHECK(Format::size(1024) == "1.0 KiB");
    CHECK(Format::size(1536) == "1.5 KiB");
    CHECK(Format::size(1024ull * 1024 * 1024) == "1.0 GiB");
    CHECK(Format::itemCount(1) == "1 item");
    CHECK(Format::itemCount(1234567) == "1,234,567 items");
    CHECK(Format::itemCount(100) == "100 items");
}

static void testHistory()
{
    NavigationHistory history("/a");
    CHECK(!history.canGoBack() && !history.canGoForward());
    history.visit("/a");
    CHECK(!history.canGoBack());
    history.visit("/b");
    history.visit("/c");
    CHECK(history.goBack() && history.current() == "/b");
    CHECK(history.goBack() && history.current() == "/a");
    CHECK(!history.goBack());
    CHECK(history.goForward() && history.current() == "/b");
    history.visit("/d");
    CHECK(!history.canGoForward());
    CHECK(history.goBack() && history.current() == "/b");
}

static void testPathBar()
{
    CHECK(PathBar::normalize("/a//b/../c/") == "/a/c");
    CHECK(PathBar::normalize("/") == "/");
    CHECK(PathBar::parent("/a/b") == "/a");
    CHECK(PathBar::parent("/a") == "/");
    CHECK(PathBar::parent("/") == "/");
    CHECK(PathBar::baseName("/a/b") == "b");

    const auto parts = PathBar::split("/home/me/My Files");
    CHECK(parts.size() == 4);
    CHECK(parts[0].label == "/" && parts[0].path == "/");
    CHECK(parts[3].label == "My Files" && parts[3].path == "/home/me/My Files");
    CHECK(PathBar::split("/").size() == 1);

    const std::string home = PathBar::homeDirectory();
    CHECK(PathBar::resolve("~", "/x") == home);
    CHECK(PathBar::resolve("~/Music", "/x") == PathBar::normalize(home + "/Music"));
    CHECK(PathBar::resolve("sub/dir", "/x") == "/x/sub/dir");
    CHECK(PathBar::resolve("..", "/x/y") == "/x");
    CHECK(PathBar::resolve("  /etc/  ", "/x") == "/etc");
    CHECK(PathBar::resolve("file:///tmp/a%20b", "/x") == "/tmp/a b");
    CHECK(PathBar::resolve("", "/x") == "/x");
    CHECK(PathBar::tabTitle(home) == "Home");
    CHECK(PathBar::tabTitle("/") == "/");
}

static void testPlaces()
{
    const Place usb = Places::driveFromMountInfo(
        "36 25 8:17 / /run/media/me/My\\040USB rw,nosuid shared:1 - ext4 /dev/sdb1 rw");
    CHECK(usb.name == "My USB" && usb.path == "/run/media/me/My USB");

    CHECK(Places::driveFromMountInfo("1 2 0:3 / /proc rw - proc proc rw").path.empty());
    CHECK(Places::driveFromMountInfo("1 2 8:2 / /boot/efi rw - vfat /dev/sda1 rw").path.empty());
    CHECK(Places::driveFromMountInfo("1 2 8:2 / / rw - btrfs /dev/sda3 rw").path.empty());
    CHECK(!Places::userPlaces().empty());
}

static void testFolderView()
{
    FolderView view;
    std::vector<Entry> entries {fileEntry("dir", 0, true), fileEntry("a.txt", 10), fileEntry("b.txt", 20),
                                fileEntry("c.txt", 40), fileEntry(".hidden", 80)};
    sortEntries(entries, view.sort());
    view.setEntries(entries, view.sort());
    CHECK(view.count() == 4);                 // .hidden is left out

    view.setShowHidden(true);
    CHECK(view.count() == 5);
    view.setShowHidden(false);

    view.click(1, ClickMode::Replace);        // a.txt
    CHECK(view.selectedCount() == 1 && view.isSelected(1) && view.currentRow() == 1);

    view.click(3, ClickMode::Extend);
    CHECK(view.selectedCount() == 3 && view.selectedSize() == 70);

    view.click(2, ClickMode::Toggle);
    CHECK(view.selectedCount() == 2 && !view.isSelected(2));

    view.selectAll();
    CHECK(view.selectedCount() == 4);
    CHECK(view.selectedSize() == 70);         // the folder adds nothing

    // the selection follows the name through a new sort order
    view.selectName("c.txt");
    view.setSort({SortKey::Name, false});
    CHECK(view.at(view.currentRow()).name == "c.txt" && view.isSelected(view.currentRow()));

    // and through a refresh, unless the file has gone
    std::vector<Entry> fewer {fileEntry("dir", 0, true), fileEntry("a.txt", 10)};
    view.setEntries(fewer, {});
    CHECK(view.selectedCount() == 0 && view.currentRow() == -1);

    view.moveCurrent(99, false);
    CHECK(view.currentRow() == 1);
    view.moveCurrent(-5, true);
    CHECK(view.currentRow() == 0 && view.selectedCount() == 2);

    CHECK(view.findPrefix("A", 0) >= 0);
    CHECK(view.findPrefix("zzz", 0) == -1);

    view.clear();
    CHECK(view.count() == 0 && view.selectedCount() == 0);
    view.moveCurrent(1, false);               // must not crash on an empty list
    view.selectAll();
    CHECK(view.findPrefix("a", 0) == -1);
}

static void testTabs()
{
    TabManager tabs;
    tabs.open("/a", 0);
    CHECK(tabs.count() == 1 && tabs.current() == 0);
    tabs.open("/b", 1);
    tabs.open("/c", 2);
    tabs.setCurrent(1);

    tabs.open("/before", 0);
    CHECK(tabs.current() == 2 && tabs.at(2)->history.current() == "/b");

    tabs.move(2, 0);
    CHECK(tabs.current() == 0 && tabs.at(0)->history.current() == "/b");
    tabs.move(3, 0);
    CHECK(tabs.current() == 1);
    tabs.move(0, 3);
    CHECK(tabs.current() == 0);

    tabs.setCurrent(tabs.count() - 1);
    const std::string last = tabs.at(tabs.current())->history.current();
    tabs.close(tabs.count() - 1);
    CHECK(tabs.count() == 3 && tabs.current() == 2);
    CHECK(tabs.canReopen());
    const int place = tabs.reopenIndex();
    CHECK(place == 3);
    CHECK(tabs.reopen()->history.current() == last);
    CHECK(tabs.count() == 4 && !tabs.canReopen());

    tabs.setCurrent(2);
    tabs.close(0);
    CHECK(tabs.current() == 1);
}

static void testDirectory(const fs::path& root)
{
    const fs::path folder = root / "listing";
    fs::create_directories(folder / "sub");
    touch(folder / "file10.txt", "0123456789");
    touch(folder / "file2.txt");
    touch(folder / ".secret");
    fs::create_symlink(folder / "sub", folder / "good-link");
    fs::create_symlink(folder / "missing", folder / "bad-link");

    ListResult result = listDirectory(folder.string(), {});
    CHECK(result.ok);
    CHECK(result.disk.known);
    CHECK(result.entries.size() == 6);
    CHECK(result.entries[0].name == "good-link" && result.entries[0].symlink && result.entries[0].isDirectory());
    CHECK(result.entries[1].name == "sub");

    int order2 = -1, order10 = -1;
    bool sawBroken = false, sawHidden = false;
    for (int i = 0; i < static_cast<int>(result.entries.size()); ++i) {
        const Entry& e = result.entries[static_cast<std::size_t>(i)];
        if (e.name == "file2.txt") order2 = i;
        if (e.name == "file10.txt") { order10 = i; CHECK(e.size == 10); CHECK(e.path == (folder / "file10.txt").string()); }
        if (e.name == "bad-link") { sawBroken = e.brokenLink; CHECK(e.typeName == "Broken Link"); }
        if (e.name == ".secret") sawHidden = e.hidden;
    }
    CHECK(order2 >= 0 && order2 < order10);
    CHECK(sawBroken && sawHidden);

    const ListResult missing = listDirectory((folder / "nope").string(), {});
    CHECK(!missing.ok && missing.error == "This folder doesn't exist.");

    const ListResult notFolder = listDirectory((folder / "file2.txt").string(), {});
    CHECK(!notFolder.ok && notFolder.error == "This is not a folder.");

    if (geteuid() != 0) {                     // root ignores permissions
        const fs::path locked = root / "locked";
        fs::create_directories(locked);
        chmod(locked.c_str(), 0);
        const ListResult denied = listDirectory(locked.string(), {});
        CHECK(!denied.ok && denied.error == "You don't have permission to open this folder.");
        chmod(locked.c_str(), 0755);
    }

    const ListResult cancelled = listDirectory(folder.string(), {}, [] { return true; });
    CHECK(!cancelled.ok);

    CHECK(PathBar::subfolders(folder.string(), false).size() == 2);   // sub and good-link
    const auto matches = PathBar::suggestions(folder.string() + "/su", "/", false, 10);
    CHECK(matches.size() == 1 && matches[0].label == "sub");
    CHECK(PathBar::suggestions(folder.string() + "/", "/", false, 1).size() == 1);
}

static void testLister(const fs::path& root)
{
    const fs::path folder = root / "async";
    fs::create_directories(folder);
    touch(folder / "one");

    DirectoryLister lister;
    std::promise<ListResult> done;
    lister.start(folder.string(), {}, [&done](ListResult result) { done.set_value(std::move(result)); });
    auto future = done.get_future();
    CHECK(future.wait_for(std::chrono::seconds(5)) == std::future_status::ready);
    CHECK(future.get().entries.size() == 1);

    // only the last of several quick starts answers
    std::atomic<int> calls {0};
    std::atomic<int> lastAnswer {0};
    for (int i = 1; i <= 50; ++i)
        lister.start(folder.string(), {}, [&, i](ListResult) { ++calls; lastAnswer = i; });
    for (int i = 0; i < 500 && lastAnswer != 50; ++i) std::this_thread::sleep_for(std::chrono::milliseconds(2));
    CHECK(lastAnswer == 50);
}

static void testWatcher(const fs::path& root)
{
    const fs::path folder = root / "watched";
    fs::create_directories(folder);

    std::atomic<int> changes {0};
    FolderWatcher watcher;
    watcher.watch(folder.string(), [&changes] { ++changes; });

    for (int i = 0; i < 5; ++i) touch(folder / ("burst" + std::to_string(i)));
    for (int i = 0; i < 300 && changes == 0; ++i) std::this_thread::sleep_for(std::chrono::milliseconds(5));
    CHECK(changes >= 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(400));
    CHECK(changes <= 2);                      // a burst is not reported file by file

    watcher.stop();
    const int before = changes;
    touch(folder / "after-stop");
    std::this_thread::sleep_for(std::chrono::milliseconds(400));
    CHECK(changes == before);

    watcher.watch((folder / "does-not-exist").string(), [] {});   // must not crash
    watcher.stop();
}

static void testTrash(const fs::path& root)
{
    setenv("XDG_DATA_HOME", (root / "data").c_str(), 1);     // never the real Trash

    const std::string files = Trash::filesDirectory();
    CHECK(files == (root / "data" / "Trash" / "files").string());
    CHECK(!PathBar::isDirectory(files));
    CHECK(Trash::ensureExists());
    CHECK(PathBar::isDirectory(files) && PathBar::isDirectory((root / "data" / "Trash" / "info").string()));
    CHECK(Trash::ensureExists());                            // a second time changes nothing

    struct stat info {};
    CHECK(stat((root / "data" / "Trash").c_str(), &info) == 0 && (info.st_mode & 0777) == 0700);

    CHECK(PathBar::tabTitle(PathBar::kTrashLocation) == "Trash");
    CHECK(PathBar::split(PathBar::kTrashLocation).size() == 1);
    CHECK(PathBar::parent(PathBar::kTrashLocation) == PathBar::kTrashLocation);
    const auto places = Places::userPlaces();
    CHECK(places.back().kind == "trash" && places.back().path == PathBar::kTrashLocation);

    // into the Trash and back, with a name that needs encoding
    const fs::path work = root / "trash-work";
    fs::create_directories(work);
    const std::string file = (work / "my report 100%.txt").string();
    touch(file, "precious");
    std::string trashed;
    CHECK(Trash::moveToTrash(file, &trashed).ok);
    CHECK(!fs::exists(file) && fs::exists(trashed));

    const auto items = Trash::list();
    CHECK(items.size() == 1 && items[0].name == "my report 100%.txt" && items[0].originalPath == file);
    CHECK(items[0].deletedAt > 1000000000);
    CHECK(Trash::contains(trashed) && !Trash::contains(file));

    const ListResult listed = listDirectory(PathBar::kTrashLocation, {});
    CHECK(listed.ok && listed.entries.size() == 1);
    CHECK(listed.entries[0].name == "my report 100%.txt" && listed.entries[0].location == work.string());
    CHECK(listed.entries[0].path == trashed);

    CHECK(!Trash::moveToTrash(trashed).ok);                  // already there
    std::string back;
    CHECK(Trash::restore(trashed, &back).ok && back == file && fs::exists(file) && !fs::exists(trashed));
    CHECK(Trash::list().empty());

    // restoring never overwrites; and the same name twice gets two places
    CHECK(Trash::moveToTrash(file, &trashed).ok);
    touch(file, "newer");
    CHECK(!Trash::restore(trashed).ok && fs::exists(trashed));
    std::string second;
    CHECK(Trash::moveToTrash(file, &second).ok && second != trashed);
    CHECK(Trash::list().size() == 2);

    // a whole folder, restored after its parent was removed
    fs::create_directories(work / "gone" / "sub");
    touch(work / "gone" / "sub" / "x", "x");
    CHECK(Trash::moveToTrash((work / "gone" / "sub").string(), &trashed).ok);
    fs::remove_all(work / "gone");
    CHECK(Trash::restore(trashed).ok && fs::exists(work / "gone" / "sub" / "x"));

    CHECK(Trash::remove(second).ok && !fs::exists(second));
    touch(file, "still here");
    CHECK(!Trash::remove(file).ok && fs::exists(file));      // not in the Trash: refused
    CHECK(Trash::empty().ok && Trash::list().empty());
    CHECK(fs::exists(file));

    // protected folders
    CHECK(!Trash::moveToTrash("/").ok);
    CHECK(!Trash::moveToTrash("/usr").ok);
    CHECK(!Trash::moveToTrash(PathBar::homeDirectory()).ok);
}

static std::string rowValue(const std::vector<PropertySection>& sections, const std::string& title, const std::string& label)
{
    for (const PropertySection& section : sections)
        if (section.title == title)
            for (const PropertyRow& row : section.rows)
                if (row.label == label) return row.value;
    return "<missing>";
}

static std::string rowValue(const std::vector<PropertyRow>& rows, const std::string& label)
{
    return rowValue({{"x", rows}}, "x", label);
}

static void testHashing(const fs::path& root)
{
    CHECK(Hashing::hashText("MD5", "abc") == "900150983cd24fb0d6963f7d28e17f72");
    CHECK(Hashing::hashText("SHA-1", "abc") == "a9993e364706816aba3e25717850c26c9cd0d89d");
    CHECK(Hashing::hashText("SHA-256", "abc") == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    CHECK(Hashing::hashText("SHA-512", "abc") == "ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a"
                                                 "2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f");
    CHECK(Hashing::hashText("BLAKE3", "abc") == "6437b3ac38465133ffb63b75273a8db548c558465d79db03fd359c6cd5bd9d85");
    CHECK(Hashing::hashText("MD5", "") == "d41d8cd98f00b204e9800998ecf8427e");
    CHECK(Hashing::hashText("SHA-256", std::string(1000, 'a')) == "41edece42d63e8d9bf515a9ba6932e1c20cbc9f5a5d134645adb5db1b9737ea3");
    CHECK(Hashing::hashText("SHA-512", std::string(200, 'a')) == Hashing::hashText("SHA-512", std::string(200, 'a')));

    const fs::path file = root / "hash-me";
    touch(file, "abc");
    std::uint64_t seen = 0;
    const auto results = Hashing::hashFile(file.string(), [&seen](std::uint64_t done, std::uint64_t) { seen = done; }, {});
    CHECK(results.size() == 5 && seen == 3);
    CHECK(results[2].name == "SHA-256" && results[2].value == Hashing::hashText("SHA-256", "abc"));
    CHECK(Hashing::hashFile(file.string(), {}, [] { return true; }).empty());
    CHECK(Hashing::hashFile((root / "missing").string(), {}, {}).empty());
}

static void testPermissions()
{
    CHECK(Permissions::modeText(S_IFREG | 0644) == "-rw-r--r--");
    CHECK(Permissions::modeText(S_IFDIR | 01777) == "drwxrwxrwt");
    CHECK(Permissions::modeText(S_IFREG | 04755) == "-rwsr-xr-x");
    CHECK(Permissions::octalText(S_IFDIR | 0755) == "0755");

    // version 2, then user::rw-, user 0:r--, group::r--, mask::r--, other::---
    const unsigned char acl[] = {2, 0, 0, 0,
                                 1, 0, 6, 0, 255, 255, 255, 255,
                                 2, 0, 4, 0, 0, 0, 0, 0,
                                 4, 0, 4, 0, 255, 255, 255, 255,
                                 16, 0, 4, 0, 255, 255, 255, 255,
                                 32, 0, 0, 0, 255, 255, 255, 255};
    const auto lines = Permissions::parseAcl(std::string(reinterpret_cast<const char*>(acl), sizeof acl));
    CHECK(lines.size() == 5 && lines[0] == "user::rw-" && lines[1] == "user:root:r--" && lines[4] == "other::---");

    // cap_net_raw (bit 13) permitted and effective, revision 2
    const unsigned char caps[] = {1, 0, 0, 2, 0, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    CHECK(Permissions::parseCapabilities(std::string(reinterpret_cast<const char*>(caps), sizeof caps)) == "cap_net_raw=ep");
}

static void testProperties(const fs::path& root)
{
    const fs::path folder = root / "props";
    fs::create_directories(folder / "inner");
    touch(folder / "a.txt", "hello world\nsecond line\n");
    touch(folder / "inner" / ".hidden", "12345");
    fs::create_symlink("a.txt", folder / "link");
    fs::create_symlink("nothing", folder / "bad");
    fs::create_hard_link(folder / "a.txt", folder / "hard");
    chmod((folder / "a.txt").c_str(), 0640);

    const auto file = Properties::collect((folder / "a.txt").string());
    CHECK(rowValue(file, "General", "Name") == "a.txt");
    CHECK(rowValue(file, "General", "Size") == "24 bytes");
    CHECK(rowValue(file, "General", "Type") == "Text File");
    CHECK(rowValue(file, "Permissions", "Octal") == "0640");
    CHECK(rowValue(file, "Permissions", "Mode") == "-rw-r-----");
    CHECK(rowValue(file, "Permissions", "Others can") == "None");
    CHECK(rowValue(file, "Filesystem", "Hard links") == "2");
    CHECK(rowValue(file, "Filesystem", "Filesystem type") != "<missing>");

    const auto link = Properties::collect((folder / "link").string());
    CHECK(rowValue(link, "Link", "Points to") == "a.txt");
    CHECK(rowValue(link, "Link", "Status") == "Working");
    CHECK(rowValue(link, "Link", "Final target") == fs::canonical(folder / "a.txt").string());
    CHECK(rowValue(Properties::collect((folder / "bad").string()), "Link", "Status").rfind("Broken", 0) == 0);

    CHECK(rowValue(Properties::collect((folder / "gone").string()), "General", "Error") != "<missing>");

    const auto selection = Properties::collectSelection({(folder / "a.txt").string(), (folder / "inner").string()});
    CHECK(rowValue(selection, "General", "Files") == "1" && rowValue(selection, "General", "Folders") == "1");
    CHECK(rowValue(selection, "General", "Location") == folder.string());
    CHECK(rowValue(selection, "General", "Mode").rfind("Mixed", 0) == 0);

    const FolderScan scan = scanFolder(folder.string(), {}, {});
    CHECK(scan.finished && scan.files == 3 && scan.folders == 1 && scan.links == 2 && scan.hidden == 1);
    CHECK(scan.totalSize == 53 && scan.largest.size() == 3 && scan.largest[0].first == 24);
    CHECK(!scanFolder(folder.string(), {}, [] { return true; }).finished);
}

static void testContent(const fs::path& root)
{
    const fs::path text = root / "t.txt";
    touch(text, "one two\r\nthree\n");
    const auto rows = ContentInfo::text(text.string());
    CHECK(rowValue(rows, "Lines") == "2" && rowValue(rows, "Words") == "3");
    CHECK(rowValue(rows, "Line endings") == "LF (1), CRLF (1)");

    // a 2 by 3 PNG: only its header matters here
    std::string png("\x89PNG\r\n\x1a\n\0\0\0\rIHDR\0\0\0\2\0\0\0\3\x08\x06\0\0\0", 33);
    touch(root / "p.png", png);
    CHECK(rowValue(ContentInfo::image((root / "p.png").string()), "Dimensions") == "2 × 3 pixels");

    const auto self = ContentInfo::elf("/proc/self/exe");
    CHECK(rowValue(self, "Architecture") == "x86-64");
    CHECK(rowValue(self, "Interpreter").find("ld-linux") != std::string::npos);
    CHECK(rowValue(self, "Linking") == "Dynamic");
}

static void testProcess()
{
    CHECK(!Process::findProgram("sh").empty());
    CHECK(Process::findProgram("no-such-program-here").empty());
    const auto echo = Process::run({"sh", "-c", "printf hello; printf oops >&2; exit 3"});
    CHECK(echo.started && echo.exitCode == 3 && echo.output == "hello" && echo.errors == "oops");
    CHECK(Process::run({"cat"}, {}, "piped").output == "piped");
    CHECK(!Process::run({"no-such-program-here"}).started);
    const auto stopped = Process::run({"sleep", "30"}, [] { return true; });
    CHECK(stopped.cancelled);
    CHECK(!Process::startDetached({"no-such-program-here"}));
    CHECK(Process::startDetached({"true"}));
}

static std::string contentOf(const fs::path& path)
{
    std::ifstream file(path);
    return std::string(std::istreambuf_iterator<char>(file), {});
}

static Transfer::Events answering(ConflictChoice choice, int* asked = nullptr)
{
    Transfer::Events events;
    events.conflict = [choice, asked](const std::string&, const std::string&) {
        if (asked) ++*asked;
        return ConflictAnswer {choice, false};
    };
    return events;
}

static void testFileOps(const fs::path& root)
{
    const fs::path work = root / "ops";
    fs::create_directories(work);
    const std::string base = work.string();

    CHECK(FileOps::createFolder(base + "/new").ok);
    CHECK(!FileOps::createFolder(base + "/new").ok);
    CHECK(FileOps::createFile(base + "/a.txt").ok);
    touch(work / "a.txt", "keep me");
    CHECK(!FileOps::createFile(base + "/a.txt").ok);
    CHECK(contentOf(work / "a.txt") == "keep me");

    touch(work / "b.txt", "other");
    CHECK(!FileOps::rename(base + "/a.txt", base + "/b.txt").ok);      // never overwrites
    CHECK(contentOf(work / "b.txt") == "other" && contentOf(work / "a.txt") == "keep me");
    CHECK(FileOps::rename(base + "/a.txt", base + "/c.txt").ok && fs::exists(work / "c.txt"));

    CHECK(FileOps::uniquePath(base + "/free.txt") == base + "/free.txt");
    CHECK(FileOps::uniquePath(base + "/c.txt") == base + "/c (2).txt");
    touch(work / "d.tar.gz");
    CHECK(FileOps::uniquePath(base + "/d.tar.gz") == base + "/d (2).tar.gz");
    touch(work / ".hidden");
    CHECK(FileOps::uniquePath(base + "/.hidden") == base + "/.hidden (2)");

    CHECK(!FileOps::nameProblem("").empty() && !FileOps::nameProblem("a/b").empty() && !FileOps::nameProblem("..").empty());
    CHECK(FileOps::nameProblem("fine name.txt").empty());

    CHECK(FileOps::isProtected("/") && FileOps::isProtected("/usr") && FileOps::isProtected("/usr/") && FileOps::isProtected("/home"));
    CHECK(FileOps::isProtected(PathBar::homeDirectory()));
    CHECK(!FileOps::isProtected(PathBar::homeDirectory() + "/Music"));
    CHECK(!FileOps::removeTree("/", {}).ok);

    // removeTree deletes a link, never what it points to
    const fs::path outside = root / "outside";
    fs::create_directories(outside);
    touch(outside / "safe", "safe");
    fs::create_directories(work / "tree" / "deep");
    touch(work / "tree" / "deep" / "f", "x");
    fs::create_directory_symlink(outside, work / "tree" / "link");
    CHECK(FileOps::removeTree((work / "tree").string(), {}).ok);
    CHECK(!fs::exists(work / "tree") && contentOf(outside / "safe") == "safe");

    fs::create_directories(work / "stop");
    touch(work / "stop" / "f");
    CHECK(!FileOps::removeTree((work / "stop").string(), [] { return true; }).ok);
    CHECK(fs::exists(work / "stop" / "f"));

    fs::create_directory_symlink(outside, work / "dirlink");
    CHECK(FileOps::removeTree((work / "dirlink").string(), {}).ok);
    CHECK(contentOf(outside / "safe") == "safe" && !fs::is_symlink(work / "dirlink"));
}

static void noPartFiles(const fs::path& folder)
{
    for (const auto& item : fs::recursive_directory_iterator(folder))
        CHECK(item.path().filename().string().rfind(".chaoticfiles-part", 0) != 0);
}

static void testTransfer(const fs::path& root)
{
    const fs::path from = root / "from", to = root / "to";
    const auto reset = [&] {
        fs::remove_all(from);
        fs::remove_all(to);
        fs::create_directories(from / "folder" / "inner");
        fs::create_directories(to);
        touch(from / "file.txt", "hello");
        touch(from / "folder" / "inner" / "deep.txt", "deep");
        fs::create_symlink("file.txt", from / "link");
        chmod((from / "file.txt").c_str(), 0640);
    };
    const auto item = [&](const char* name) { return (from / name).string(); };
    const std::string dest = to.string();

    // copy keeps content, mode, time, links as links
    reset();
    Transfer copy(TransferKind::Copy, {item("file.txt"), item("folder"), item("link")}, dest, {});
    TransferResult result = copy.run();
    CHECK(result.errors.empty() && result.done.size() == 3);
    CHECK(contentOf(to / "file.txt") == "hello" && contentOf(to / "folder" / "inner" / "deep.txt") == "deep");
    CHECK((fs::status(to / "file.txt").permissions() & fs::perms::others_read) == fs::perms::none);
    CHECK(fs::last_write_time(to / "file.txt") == fs::last_write_time(from / "file.txt"));
    CHECK(fs::is_symlink(to / "link") && fs::read_symlink(to / "link") == "file.txt");
    CHECK(fs::exists(from / "file.txt"));
    noPartFiles(to);

    // the same, with progress reported to the end
    reset();
    std::uint64_t lastDone = 0, total = 0;
    Transfer::Events counting;
    counting.progress = [&](const TransferProgress& p) { lastDone = p.bytesDone; total = p.bytesTotal; };
    Transfer counted(TransferKind::Copy, {item("folder")}, dest, counting);
    counted.run();
    CHECK(total == 4 && lastDone == 4);

    // conflicts: skip leaves the old file, overwrite replaces it, keep both makes a second
    reset();
    touch(to / "file.txt", "old");
    int asked = 0;
    Transfer skip(TransferKind::Copy, {item("file.txt")}, dest, answering(ConflictChoice::Skip, &asked));
    result = skip.run();
    CHECK(asked == 1 && result.skipped == 1 && contentOf(to / "file.txt") == "old");

    Transfer keep(TransferKind::Copy, {item("file.txt")}, dest, answering(ConflictChoice::KeepBoth));
    keep.run();
    CHECK(contentOf(to / "file.txt") == "old" && contentOf(to / "file (2).txt") == "hello");

    Transfer over(TransferKind::Copy, {item("file.txt")}, dest, answering(ConflictChoice::Overwrite));
    over.run();
    CHECK(contentOf(to / "file.txt") == "hello");

    touch(to / "file.txt", "old");
    Transfer stop(TransferKind::Copy, {item("file.txt")}, dest, answering(ConflictChoice::Cancel));
    result = stop.run();
    CHECK(result.cancelled && contentOf(to / "file.txt") == "old");

    // no way to answer counts as cancel, never as overwrite
    Transfer silent(TransferKind::Copy, {item("file.txt")}, dest, {});
    silent.run();
    CHECK(contentOf(to / "file.txt") == "old");

    // a file can't replace a folder or the other way round
    reset();
    fs::create_directories(to / "file.txt");
    touch(to / "file.txt" / "inside", "safe");
    Transfer clash(TransferKind::Copy, {item("file.txt")}, dest, answering(ConflictChoice::Overwrite));
    result = clash.run();
    CHECK(!result.errors.empty() && contentOf(to / "file.txt" / "inside") == "safe");

    // folders merge
    reset();
    fs::create_directories(to / "folder" / "inner");
    touch(to / "folder" / "mine.txt", "mine");
    touch(to / "folder" / "inner" / "deep.txt", "old deep");
    Transfer merge(TransferKind::Copy, {item("folder")}, dest, answering(ConflictChoice::Skip));
    result = merge.run();
    CHECK(contentOf(to / "folder" / "mine.txt") == "mine" && contentOf(to / "folder" / "inner" / "deep.txt") == "old deep");
    CHECK(result.done.empty());                           // the folder was already there: undo must leave it alone

    fs::remove(to / "folder" / "inner" / "deep.txt");
    Transfer cleanMerge(TransferKind::Copy, {item("folder")}, dest, {});
    result = cleanMerge.run();
    CHECK(result.done.empty() && result.merged == 1 && contentOf(to / "folder" / "inner" / "deep.txt") == "deep");

    // a move: instant on one disk, and the source goes
    reset();
    Transfer move(TransferKind::Move, {item("file.txt"), item("folder")}, dest, {});
    result = move.run();
    CHECK(result.errors.empty() && result.done.size() == 2);
    CHECK(!fs::exists(from / "file.txt") && !fs::exists(from / "folder"));
    CHECK(contentOf(to / "folder" / "inner" / "deep.txt") == "deep");

    // a move that skips a conflicting file keeps that file and its folder
    reset();
    fs::create_directories(to / "folder" / "inner");
    touch(to / "folder" / "inner" / "deep.txt", "old");
    Transfer partial(TransferKind::Move, {item("folder")}, dest, answering(ConflictChoice::Skip));
    partial.run();
    CHECK(contentOf(from / "folder" / "inner" / "deep.txt") == "deep" && contentOf(to / "folder" / "inner" / "deep.txt") == "old");

    // a folder into itself is refused, and nothing changes
    reset();
    Transfer self(TransferKind::Copy, {item("folder")}, (from / "folder" / "inner").string(), {});
    result = self.run();
    CHECK(result.errors.size() == 1 && !fs::exists(from / "folder" / "inner" / "folder"));
    Transfer selfMove(TransferKind::Move, {item("folder")}, item("folder"), {});
    CHECK(selfMove.run().errors.size() == 1 && fs::exists(from / "folder" / "inner"));

    // pasting into the same folder: a copy gets a new name, a move does nothing
    reset();
    Transfer twin(TransferKind::Copy, {item("file.txt")}, from.string(), {});
    twin.run();
    CHECK(contentOf(from / "file (2).txt") == "hello");
    Transfer still(TransferKind::Move, {item("file.txt")}, from.string(), {});
    still.run();
    CHECK(contentOf(from / "file.txt") == "hello");

    // a missing destination
    Transfer lost(TransferKind::Copy, {item("file.txt")}, (root / "nowhere").string(), {});
    CHECK(!lost.run().errors.empty());

    // cancelling leaves no half file; a big enough file to be stopped part way
    reset();
    touch(from / "big", std::string(8 * 1024 * 1024, 'x'));
    Transfer::Events cancelling;
    Transfer* running = nullptr;
    cancelling.progress = [&running](const TransferProgress&) { if (running) running->cancel(); };
    Transfer stopped(TransferKind::Copy, {item("big"), item("file.txt")}, dest, cancelling);
    running = &stopped;
    result = stopped.run();
    CHECK(result.cancelled);
    noPartFiles(to);
    CHECK(fs::exists(from / "big"));

    // pause holds the work back until resumed
    reset();
    Transfer paused(TransferKind::Copy, {item("file.txt")}, dest, {});
    paused.setPaused(true);
    std::atomic<bool> finished {false};
    std::thread worker([&] { paused.run(); finished = true; });
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    CHECK(!finished);
    paused.setPaused(false);
    worker.join();
    CHECK(contentOf(to / "file.txt") == "hello");

    // across disks a move is copy then delete: /dev/shm and the test folder may differ
    if (fs::exists("/dev/shm") && !FileOps::onSameDevice("/dev/shm", root.string())) {
        char pattern[] = "/dev/shm/chaoticfiles-tests-XXXXXX";
        if (const char* made = mkdtemp(pattern)) {
            reset();
            Transfer across(TransferKind::Move, {item("folder"), item("link")}, made, {});
            result = across.run();
            CHECK(result.errors.empty());
            CHECK(!fs::exists(from / "folder") && contentOf(fs::path(made) / "folder" / "inner" / "deep.txt") == "deep");
            CHECK(fs::is_symlink(fs::path(made) / "link"));
            fs::remove_all(made);
        }
    }
}

static void testUndo()
{
    UndoStack stack;
    CHECK(!stack.canUndo());
    stack.push({UndoStep::Kind::Renamed, {{"/a", "/b"}}, "Rename"});
    stack.push({UndoStep::Kind::Moved, {{"/c", "/d"}}, "Move"});
    CHECK(stack.canUndo() && stack.nextDescription() == "Move");
    CHECK(stack.take().kind == UndoStep::Kind::Moved);
    CHECK(stack.take().pairs[0].second == "/b" && !stack.canUndo());
}

static void testArchives(const fs::path& root)
{
    CHECK(Archiver::isArchive("a.tar.gz") && Archiver::isArchive("A.ZIP") && !Archiver::isArchive("a.txt"));
    CHECK(Archiver::withoutExtension("photos.tar.gz") == "photos" && Archiver::withoutExtension("a.zip") == "a");

    const fs::path work = root / "archive";
    fs::create_directories(work / "stuff");
    touch(work / "stuff" / "a b.txt", "inside");
    touch(work / "-dash.txt", "tricky name");
    const std::vector<std::string> items = {(work / "stuff").string(), (work / "-dash.txt").string()};

    for (const char* kind : {"out.tar.gz", "out.tar.xz", "out.zip", "out.7z"}) {
        const fs::path archive = work / kind;
        const Outcome made = Archiver::compress(items, archive.string(), {});
        if (!made.ok) std::printf("  %s: %s\n", kind, made.error.c_str());
        CHECK(made.ok && fs::exists(archive));
        const fs::path out = work / (std::string("x-") + kind);
        fs::create_directories(out);
        const Outcome opened = Archiver::extract(archive.string(), out.string(), {});
        if (!opened.ok) std::printf("  %s: %s\n", kind, opened.error.c_str());
        CHECK(opened.ok && contentOf(out / "stuff" / "a b.txt") == "inside" && contentOf(out / "-dash.txt") == "tricky name");
    }
    CHECK(!Archiver::compress(items, (work / "out.rar").string(), {}).ok);

    touch(work / "fake.zip", "not an archive");
    fs::create_directories(work / "fake-out");
    CHECK(!Archiver::extract((work / "fake.zip").string(), (work / "fake-out").string(), {}).ok);

    const auto info = ContentInfo::archive((work / "out.zip").string(), "application/zip", {});
    CHECK(rowValue(info, "Files inside") == "2" && rowValue(info, "Folders inside") == "1");
    const auto tarInfo = ContentInfo::archive((work / "out.tar.gz").string(), "application/gzip", {});
    CHECK(rowValue(tarInfo, "Files inside") == "2" && rowValue(tarInfo, "Folders inside") == "1");
}

static void testApps()
{
    const DesktopApp app = DesktopApps::parse("x.desktop",
        "[Desktop Entry]\nName=Viewer\nType=Application\nExec=\"/opt/my app/view\" --new %U\nMimeType=image/png;text/plain;\n"
        "[Desktop Action Other]\nName=Wrong\nExec=wrong\n");
    CHECK(app.name == "Viewer" && app.exec == "\"/opt/my app/view\" --new %U" && app.mimeTypes.size() == 2);

    const auto command = DesktopApps::commandFor(app.exec, {"/tmp/a b.png", "/tmp/c.png"});
    CHECK(command.size() == 4 && command[0] == "/opt/my app/view" && command[1] == "--new");
    CHECK(command[2] == "file:///tmp/a%20b.png");

    const auto single = DesktopApps::commandFor("mpv %f --title=%c", {"/tmp/x; rm -rf y", "/tmp/z"});
    CHECK(single.size() == 3 && single[1] == "/tmp/x; rm -rf y");
    CHECK(DesktopApps::commandFor("edit", {"/f"}).back() == "/f");

    const DesktopApp hidden = DesktopApps::parse("h.desktop", "[Desktop Entry]\nName=H\nType=Application\nExec=h\nNoDisplay=true\n");
    CHECK(hidden.exec.empty());

    CHECK(DesktopApps::parse("i.desktop", "[Desktop Entry]\nName=I\nType=Application\nExec=i\nIcon=org.app.Icon\n").icon == "org.app.Icon");

    CHECK(fileTypeLabel("text/plain") == "Plain Text");
    CHECK(fileTypeLabel("audio/x-flac") == "FLAC");
    CHECK(fileTypeLabel("application/x-unknown") == "application/x-unknown");
    for (const FileTypeGroup& group : fileTypeGroups())
        for (const FileTypeChoice& type : group.types) CHECK(!type.label.empty() && !type.mimeTypes.empty());
}

// xdg-mime reads and writes under these folders, so the real settings of whoever runs the tests stay untouched
static void testDefaultApps(const fs::path& root)
static void testDropTargets()
{
    CHECK(FileOps::alreadyIn("/a/b", "/a"));
    CHECK(FileOps::alreadyIn("/a/b", "/a/b"));
    CHECK(!FileOps::alreadyIn("/a/b", "/a/c"));
    CHECK(!FileOps::alreadyIn("/a/b/c", "/a"));
    CHECK(FileOps::alreadyIn("/a", "/"));
}

{
    if (Process::findProgram("xdg-mime").empty()) return;
    const fs::path work = root / "defaults";
    fs::create_directories(work / "config" / "gtk-3.0");
    fs::create_directories(work / "data" / "applications");
    touch(work / "data" / "applications" / "player.desktop",
          "[Desktop Entry]\nName=Player\nType=Application\nExec=/bin/sh %f\nIcon=player\nMimeType=audio/mpeg;audio/flac;\n");
    touch(work / "data" / "applications" / "other.desktop",
          "[Desktop Entry]\nName=Other\nType=Application\nExec=/bin/sh %f\nMimeType=video/mp4;\n");

    const std::string oldConfig = std::getenv("XDG_CONFIG_HOME") ? std::getenv("XDG_CONFIG_HOME") : "";
    const std::string oldData = std::getenv("XDG_DATA_HOME") ? std::getenv("XDG_DATA_HOME") : "";
    const bool hadConfig = std::getenv("XDG_CONFIG_HOME") != nullptr, hadData = std::getenv("XDG_DATA_HOME") != nullptr;
    setenv("XDG_CONFIG_HOME", (work / "config").c_str(), 1);
    setenv("XDG_DATA_HOME", (work / "data").c_str(), 1);

    CHECK(DesktopApps::setDefault("player.desktop", {"audio/mpeg", "audio/flac"}));
    CHECK(DesktopApps::setDefault("other.desktop", {"video/mp4"}));
    CHECK(!DesktopApps::setDefault("player.desktop", {}));
    CHECK(fs::exists(work / "config" / "mimeapps.list"));
    CHECK(DesktopApps::defaultFor("audio/flac") == "player.desktop");

    const auto many = DesktopApps::defaultsFor({"audio/mpeg", "video/mp4", "audio/flac"});
    CHECK(many.size() == 3 && many[0] == "player.desktop" && many[1] == "other.desktop" && many[2] == "player.desktop");
    CHECK(DesktopApps::defaultsFor({}).empty());

    const auto apps = DesktopApps::forMimeTypes({"audio/mpeg", "video/mp4"});
    CHECK(apps.size() >= 2 && apps[0].id == "player.desktop");       // the default of the first type leads

    touch(work / "note.txt", "plain words\n");
    CHECK(DesktopApps::openingType((work / "note.txt").string()) == "text/plain");

    touch(work / "config" / "gtk-3.0" / "settings.ini", "[Settings]\ngtk-theme-name=X\ngtk-icon-theme-name = \"breeze\"\n");
    CHECK(DesktopApps::iconThemeName() == "breeze");

    if (hadConfig) setenv("XDG_CONFIG_HOME", oldConfig.c_str(), 1);
    else unsetenv("XDG_CONFIG_HOME");
    if (hadData) setenv("XDG_DATA_HOME", oldData.c_str(), 1);
    else unsetenv("XDG_DATA_HOME");
}

static void testFolderIcons(const fs::path& root)
{
    CHECK(FolderIcons::make("music", "green") == "music;green");
    CHECK(FolderIcons::make("", "") == "");
    CHECK(FolderIcons::iconOf("music;green") == "music" && FolderIcons::colorOf("music;green") == "green");
    CHECK(FolderIcons::iconOf(";green") == "" && FolderIcons::colorOf(";green") == "green");
    CHECK(FolderIcons::iconOf("music") == "music" && FolderIcons::colorOf("music") == "");

    const fs::path work = root / "foldericons";
    fs::create_directories(work / "plain");
    fs::create_directories(work / "styled" / "inside");
    touch(work / "file.txt");
    fs::create_directory_symlink(work / "styled", work / "link");

    CHECK(FolderIcons::read((work / "plain").string()).empty());
    const Outcome saved = FolderIcons::write((work / "styled").string(), "music;green");
    if (!saved.ok) {                       // the temporary folder is on a filesystem without attributes
        std::printf("skipped folder icon tests: %s\n", saved.error.c_str());
        return;
    }
    CHECK(FolderIcons::read((work / "styled").string()) == "music;green");
    CHECK(FolderIcons::read((work / "link").string()) == "music;green");

    CHECK(FolderIcons::write((work / "file.txt").string(), "star;red").ok);    // a file is skipped, not changed
    CHECK(FolderIcons::read((work / "file.txt").string()).empty());
    CHECK(!FolderIcons::write((work / "missing").string(), "star;red").ok);

    // it shows in a listing, and a change counts as a change
    const auto find = [&work](const std::string& name) {
        for (Entry& entry : listDirectory(work.string(), {}).entries)
            if (entry.name == name) return entry;
        return Entry();
    };
    Entry before = find("styled");
    CHECK(before.customIcon == "music;green");
    CHECK(find("link").customIcon == "music;green");
    CHECK(find("plain").customIcon.empty());

    // it follows a rename, and a copy of the folder keeps it
    fs::rename(work / "styled", work / "renamed");
    CHECK(FolderIcons::read((work / "renamed").string()) == "music;green");
    fs::create_directories(work / "copies");
    Transfer copy(TransferKind::Copy, {(work / "renamed").string()}, (work / "copies").string(), {});
    copy.run();
    CHECK(FolderIcons::read((work / "copies" / "renamed").string()) == "music;green");

    CHECK(FolderIcons::write((work / "renamed").string(), "").ok);
    CHECK(FolderIcons::read((work / "renamed").string()).empty());
    Entry after = find("renamed");
    after.name = before.name;
    after.path = before.path;
    CHECK(!sameOnDisk(before, after));
    CHECK(FolderIcons::write((work / "plain").string(), "").ok);               // nothing to remove is fine
}

static void testSearch(const fs::path& root)
{
    CHECK(FileSearch::nameMatches("Holiday Photo.PNG", "photo"));
    CHECK(!FileSearch::nameMatches("notes.txt", "photo"));

    const fs::path work = root / "search";
    fs::create_directories(work / "a" / "b");
    fs::create_directories(work / ".secret");
    touch(work / "report.txt");
    touch(work / "a" / "REPORT-2.pdf");
    touch(work / "a" / "b" / "old report");
    touch(work / ".secret" / "report-hidden");
    touch(work / "a" / "other.txt");
    fs::create_directory_symlink(work / "a", work / "loop");

    std::vector<Entry> found;
    const auto collect = [&found](std::vector<Entry> batch) {
        found.insert(found.end(), batch.begin(), batch.end());
    };
    FileSearch::run(work.string(), "report", false, 100, collect, {});
    CHECK(found.size() == 3);                       // no hidden, and the link is not walked
    for (const Entry& entry : found) CHECK(entry.location == PathBar::parent(entry.path));

    found.clear();
    FileSearch::run(work.string(), "report", true, 100, collect, {});
    CHECK(found.size() == 4);

    found.clear();
    FileSearch::run(work.string(), "report", false, 2, collect, {});
    CHECK(found.size() == 2);

    found.clear();
    FileSearch::run(work.string(), "report", false, 100, collect, [] { return true; });
    CHECK(found.empty());

    FolderView view;
    view.setEntries({fileEntry("apple"), fileEntry("banana"), fileEntry("cherry")}, view.sort());
    view.setFilter("AN");
    CHECK(view.count() == 1 && view.at(0).name == "banana");
    view.setFilter("");
    CHECK(view.count() == 3);
    view.addEntries({fileEntry("avocado")});
    CHECK(view.count() == 4 && view.at(1).name == "avocado");
    view.selectRows({0, 2}, false);
    CHECK(view.selectedCount() == 2);
    view.selectRows({3}, true);
    CHECK(view.selectedCount() == 3);
    view.selectRows({99, -1}, false);
    CHECK(view.selectedCount() == 0);
}

static void testRecent(const fs::path& root)
{
    setenv("XDG_DATA_HOME", (root / "recent-data").c_str(), 1);
    fs::create_directories(root / "recent-data");
    const fs::path a = root / "recent-a", b = root / "recent b";
    touch(a);
    touch(b);
    RecentFiles::add(a.string());
    RecentFiles::add(b.string());
    RecentFiles::add(a.string());
    auto items = RecentFiles::list();
    CHECK(items.size() == 2 && items[0] == a.string());

    fs::remove(a);
    items = RecentFiles::list();
    CHECK(items.size() == 1 && items[0] == b.string());

    const auto parsed = RecentFiles::parseXbel("<bookmark href=\"file:///tmp/a%20b.txt\" added=\"x\"><bookmark href=\"https://x\"/>"
                                               "<bookmark href=\"file:///tmp/c%26d\"/>");
    CHECK(parsed.size() == 2 && parsed[0] == "/tmp/a b.txt" && parsed[1] == "/tmp/c&d");

    const ListResult listed = listDirectory(PathBar::kRecentLocation, {});
    CHECK(listed.ok && listed.entries.size() == 1 && listed.entries[0].location == root.string());
    RecentFiles::clear();
    CHECK(RecentFiles::list().empty());
}

static void testShortcuts()
{
    ShortcutMap map;
    CHECK(map.keysOf("copy").size() == 1 && map.keysOf("copy")[0] == "Ctrl+C");
    CHECK(map.keysOf("nextTab").size() == 2);
    CHECK(map.keysOf("nothing").empty());

    map.set("copy", "Ctrl+Y");
    CHECK(map.keysOf("copy")[0] == "Ctrl+Y" && map.isCustom("copy"));

    map.set("paste", "Ctrl+Y");                      // one key, one action
    CHECK(map.keysOf("copy").empty() && map.keysOf("paste")[0] == "Ctrl+Y");

    map.set("refresh", "");
    CHECK(map.keysOf("refresh").empty());

    map.resetAll();
    CHECK(map.keysOf("copy")[0] == "Ctrl+C" && !map.isCustom("copy"));

    // every default key is used once
    std::vector<std::string> seen;
    for (const ShortcutAction& action : map.actions())
        for (const std::string& key : action.defaults) {
            CHECK(std::find(seen.begin(), seen.end(), key) == seen.end());
            seen.push_back(key);
        }
}

static void testDrives()
{
    const auto drives = Drives::parseLsblk(
        "PATH=\"/dev/sda1\" LABEL=\"My Disk\" FSTYPE=\"ext4\" SIZE=\"1073741824\" MOUNTPOINT=\"\" TYPE=\"part\"\n"
        "PATH=\"/dev/sdb\" LABEL=\"\" FSTYPE=\"\" SIZE=\"2048\" MOUNTPOINT=\"/mnt/a b\" TYPE=\"disk\"\n");
    CHECK(drives.size() == 2);
    CHECK(drives[0].device == "/dev/sda1" && drives[0].label == "My Disk" && drives[0].size == 1073741824);
    CHECK(drives[0].displayName() == "My Disk" && drives[1].displayName() == "2.0 KiB Volume");
    CHECK(drives[1].mountPoint == "/mnt/a b");
    CHECK(Places::driveFromMountInfo("36 25 8:17 / /run/media/me/USB rw - ext4 /dev/sdb1 rw").device == "/dev/sdb1");
}

static void testNetwork()
{
    const NetworkShare smb = NetworkShares::parseMountName("smb-share:server=nas,share=files");
    CHECK(smb.name == "files on nas" && smb.uri == "smb://nas/files");
    CHECK(smb.path == NetworkShares::mountsFolder() + "/smb-share:server=nas,share=files");

    const NetworkShare sftp = NetworkShares::parseMountName("sftp:host=server.local,port=2222,user=me");
    CHECK(sftp.name == "server.local" && sftp.uri == "sftp://me@server.local:2222/");

    CHECK(NetworkShares::parseMountName("something-else").uri.empty());
    CHECK(NetworkShares::parseMountName("mtp:host=phone").uri.empty());

    CHECK(NetworkShares::normalizeAddress(" nas/files ") == "smb://nas/files");
    CHECK(NetworkShares::normalizeAddress("sftp://me@host") == "sftp://me@host");
    CHECK(NetworkShares::normalizeAddress("\\\\nas\\files") == "smb://nas/files");
}

static void testThumbnails(const fs::path& root)
{
    setenv("XDG_CACHE_HOME", (root / "cache").c_str(), 1);
    // the name the thumbnail specification gives: the MD5 of the file's URL
    CHECK(Thumbnails::cachePath("/tmp/a b.png") == (root / "cache" / "thumbnails" / "large").string() + "/"
          + Hashing::hashText("MD5", "file:///tmp/a%20b.png") + ".png");
    CHECK(Thumbnails::ensureCacheFolder() && fs::is_directory(root / "cache" / "thumbnails" / "large"));
    CHECK(!Thumbnails::generate("/nope", "other", (root / "x.png").string(), 256, {}));
}

int main()
{
    char pattern[] = "/tmp/chaoticfiles-tests-XXXXXX";
    const char* made = mkdtemp(pattern);
    if (!made) return 2;
    const fs::path root = made;

    testNaturalOrder();
    testSorting();
    testFileTypes();
    testFormat();
    testHistory();
    testPathBar();
    testPlaces();
    testFolderView();
    testTabs();
    testDirectory(root);
    testLister(root);
    testWatcher(root);
    testTrash(root);
    testHashing(root);
    testPermissions();
    testProperties(root);
    testContent(root);
    testProcess();
    testFileOps(root);
    testTransfer(root);
    testUndo();
    testArchives(root);
    testApps();
    testDefaultApps(root);
    testFolderIcons(root);
    testSearch(root);
    testRecent(root);
    testShortcuts();
    testDrives();
    testNetwork();
    testThumbnails(root);

    fs::remove_all(root);
    if (failures == 0) std::printf("All backend tests passed.\n");
    return failures == 0 ? 0 : 1;
}
    testDropTargets();
