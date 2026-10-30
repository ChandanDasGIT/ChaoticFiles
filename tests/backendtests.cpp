// Tests for the engine. No Qt and no test library: a failed CHECK prints its line and the program
// ends with a non-zero code. Files are created under a temporary folder that is removed afterwards.
#include "directory/directorylister.h"
#include "filetype/filetype.h"
#include "folderview/folderview.h"
#include "format/format.h"
#include "navigation/navigationhistory.h"
#include "pathbar/pathbar.h"
#include "places/places.h"
#include "tabs/tabmanager.h"
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

    fs::remove_all(root);
    if (failures == 0) std::printf("All backend tests passed.\n");
    return failures == 0 ? 0 : 1;
}
