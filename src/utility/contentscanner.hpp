#pragma once

#include <tinydir.h>

#include <fstream>
#include <functional>

namespace agdg {
	class ContentScanner {
	public:
		// TODO: move this?
		static std::string GetFileContents(const std::string& path) {
			std::ifstream ifs(path);
			// FIXME: what if open fails?
			return std::string(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
		}

		static void ScanDirectory(const std::string& path, std::function<void(const std::string& path, const char* filename)> callback,
				const std::string& requiredSuffix = "", bool recursive = true) {
			tinydir_dir dir;

			for (tinydir_open(&dir, path.c_str()); dir.has_next; tinydir_next(&dir)) {
				tinydir_file file;
				tinydir_readfile(&dir, &file);

				if (file.name[0] == '.')
					continue;

				if (file.is_dir) {
					if (recursive)
						ScanDirectory(path + "/" + file.name, callback, requiredSuffix, recursive);
				}
				else if (requiredSuffix.empty() || EndsWith(file.name, requiredSuffix))
					callback(path + "/" + file.name, file.name);
			}

			tinydir_close(&dir);
		}

	private:
		// http://stackoverflow.com/a/874160/2524350
		static bool EndsWith(const std::string& fullString, const std::string& ending) {
			if (fullString.length() >= ending.length())
				return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
			else
				return false;
		}
	};
}
