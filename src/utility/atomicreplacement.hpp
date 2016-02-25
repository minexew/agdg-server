#pragma once

namespace agdg {
	class AtomicReplacement {
	public:
		AtomicReplacement(const std::string& original_filename) : original_filename(original_filename),
				temp_filename(original_filename + ".tmp") {
		}

		const char* c_str() { return temp_filename.c_str(); }
		void commit() { rename(temp_filename.c_str(), original_filename.c_str()); }

	private:
		std::string original_filename, temp_filename;
	};
}
