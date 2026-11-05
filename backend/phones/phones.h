#pragma once
#include "fileops/fileops.h"

#include <string>
#include <vector>

struct Phone {
    std::string name;            // "POCO X3 Pro"
    std::string uri;             // mtp://Xiaomi_POCO_X3_Pro_6a91fef5/
    std::string path;            // the folder gvfs shows it as, once mounted
};

// Phones and cameras plugged in by USB. They are not disks (MTP and PTP), so lsblk does not see them;
// gvfs does, and `gio mount` makes each one appear as a normal folder under the gvfs folder.
namespace Phones {

std::vector<Phone> parseGioList(const std::string& text, const std::string& gvfsFolder);   // `gio mount -li`
std::string folderName(const std::string& uri);              // mtp://abc/ becomes "mtp:host=abc"
std::vector<Phone> list();
Outcome mount(const std::string& uri, std::string* path);
Outcome unmount(const std::string& uri);
bool isPhoneUri(const std::string& uri);                     // mtp:// or gphoto2://

} // namespace Phones
