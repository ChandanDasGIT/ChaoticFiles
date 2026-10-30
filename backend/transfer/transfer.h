#pragma once
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>

enum class TransferKind { Copy, Move };
enum class ConflictChoice { Overwrite, Skip, KeepBoth, Cancel };

struct ConflictAnswer {
    ConflictChoice choice = ConflictChoice::Cancel;
    bool applyToAll = false;
};

struct TransferProgress {
    std::uint64_t bytesDone = 0;
    std::uint64_t bytesTotal = 0;
    std::uint64_t filesDone = 0;
    std::uint64_t filesTotal = 0;
    std::string currentName;
};

struct TransferResult {
    bool cancelled = false;
    std::vector<std::string> errors;                          // sentences for the user
    std::vector<std::pair<std::string, std::string>> done;    // top-level items: from, to
    std::uint64_t skipped = 0;
};

// Copies or moves items into a folder. run() blocks, so it goes on a background thread; the others can be
// called from any thread. The safety rules:
// - A file is written under a temporary name and renamed at the end, so a cancelled or failed copy leaves
//   no half-written file, and an overwritten file is only replaced once its new content is complete.
// - A move deletes its source only after every part of it has arrived.
// - Links are copied as links, never followed.
// - A folder can't go into itself. Folders with the same name are merged, file by file.
class Transfer {
public:
    struct Events {
        std::function<void(const TransferProgress&)> progress;
        std::function<ConflictAnswer(const std::string& source, const std::string& target)> conflict;   // may block
    };

    Transfer(TransferKind kind, std::vector<std::string> sources, std::string destination, Events events);

    TransferResult run();
    void cancel();
    void setPaused(bool paused);

private:
    enum class Decision { Write, WriteOverExisting, Skip, Stop };

    bool shouldStop();
    void report(const std::string& name, bool force = false);
    Decision resolveConflict(const std::string& source, std::string& target);
    bool processItem(const std::string& source, const std::string& target, bool topLevel);
    bool copyItem(const std::string& source, const std::string& target, bool overwrite);
    bool copyFolder(const std::string& source, const std::string& target);
    bool copyFile(const std::string& source, const std::string& target, bool overwrite);
    bool copyData(int from, int to, std::uint64_t size, const std::string& name);
    void measure(const std::string& path, std::uint64_t& bytes, std::uint64_t& files) const;
    void fail(const std::string& path, int error);

    TransferKind m_kind;
    std::vector<std::string> m_sources;
    std::string m_destination;
    Events m_events;

    std::atomic<bool> m_cancelled {false};
    bool m_paused = false;
    std::mutex m_pauseLock;
    std::condition_variable m_resume;

    std::optional<ConflictChoice> m_answerForAll;
    TransferProgress m_progress;
    TransferResult m_result;
    std::int64_t m_lastReport = 0;
};
