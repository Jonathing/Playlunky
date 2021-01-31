#include "virtual_filesystem.h"

#include "log.h"
#include "util/on_scope_exit.h"

#include <Windows.h>
#include <filesystem>

class IVfsMountImpl {
public:
	virtual ~IVfsMountImpl() = default;

	using FileInfo = VirtualFilesystem::FileInfo;
	virtual FileInfo* LoadFile(const char* file_path, void* (*allocator)(std::size_t)) const = 0;
	virtual FileInfo* LoadSubFile(const char* sub_file_path, void* (*allocator)(std::size_t), bool validate) const = 0;
	virtual std::optional<std::filesystem::path> GetFilePath(const std::filesystem::path& path) const = 0;
};

class VfsFolderMount : public IVfsMountImpl {
public:
	VfsFolderMount(std::filesystem::path mounted_path)
		: mMountedPath(std::move(mounted_path))
		, mMountedPathString(mMountedPath.string()) {
		std::replace(mMountedPathString.begin(), mMountedPathString.end(), '\\', '/');
	}
	virtual ~VfsFolderMount() override = default;

	virtual FileInfo* LoadFile(const char* file_path, void* (*allocator)(std::size_t)) const override {
		char full_path[MAX_PATH];
		sprintf_s(full_path, "%s/%s", mMountedPathString.c_str(), file_path);

		return LoadSubFile(full_path, allocator, false);
	}

	virtual FileInfo* LoadSubFile(const char* sub_file_path, void* (*allocator)(std::size_t), bool validate) const override {
		if (sub_file_path == nullptr) {
			return {};
		}

		if (validate) {
			char cleaned_sub_file_path[MAX_PATH];
			sprintf_s(cleaned_sub_file_path, "%s", sub_file_path);
			std::replace(cleaned_sub_file_path, cleaned_sub_file_path + std::strlen(cleaned_sub_file_path), '\\', '/');
			if (std::strstr(cleaned_sub_file_path, mMountedPathString.c_str()) != cleaned_sub_file_path) {
				return {};
			}
		}

		FILE* file{ nullptr };
		auto error = fopen_s(&file, sub_file_path, "rb");
		if (error == 0 && file != nullptr) {
			auto close_file = OnScopeExit{ [file]() { fclose(file); } };

			fseek(file, 0, SEEK_END);
			const std::size_t file_size = ftell(file);
			fseek(file, 0, SEEK_SET);

			if (allocator == nullptr) {
				allocator = malloc;
			}

			const std::size_t allocation_size = file_size + sizeof(FileInfo);
			if (void* buf = allocator(allocation_size)) {
				void* data = static_cast<void*>(reinterpret_cast<char*>(buf) + 24);
				const auto size_read = fread(data, 1, file_size, file);
				if (size_read != file_size) {
					LogInfo("Could not read file {}, this will either crash or cause glitches...", sub_file_path);
				}

				FileInfo* file_info = new (buf) FileInfo();
				*file_info = {
					.Data = data,
					.DataSize = static_cast<int>(file_size),
					.AllocationSize = static_cast<int>(allocation_size)
				};

				return file_info;
			}
		}

		return {};
	}

	virtual std::optional<std::filesystem::path> GetFilePath(const std::filesystem::path& path) const override {
		std::filesystem::path full_path = mMountedPath / path;
		if (std::filesystem::exists(full_path)) {
			return std::move(full_path);
		}
		return std::nullopt;
	}

private:
	std::filesystem::path mMountedPath;
	std::string mMountedPathString;
};

struct VirtualFilesystem::VfsMount {
	std::int64_t Priority;
	std::unique_ptr<IVfsMountImpl> MountImpl;
};

VirtualFilesystem::VirtualFilesystem() = default;
VirtualFilesystem::~VirtualFilesystem() = default;

void VirtualFilesystem::MountFolder(std::string_view path, std::int64_t priority) {
	namespace fs = std::filesystem;

	LogInfo("Mounting folder '{}' as a virtual filesystem...", path);

	auto it = std::upper_bound(mMounts.begin(), mMounts.end(), priority, [](std::int64_t prio, const VfsMount& mount) { return mount.Priority > prio; });
	mMounts.insert(it, VfsMount{
			.Priority = priority,
			.MountImpl = std::make_unique<VfsFolderMount>(path)
		});
}

VirtualFilesystem::FileInfo* VirtualFilesystem::LoadFile(const char* path, void* (*allocator)(std::size_t)) const {
	for (const VfsMount& mount : mMounts) {
		if (FileInfo* loaded_data = mount.MountImpl->LoadFile(path, allocator)) {
			return loaded_data;
		}
	}

	return {};
}

VirtualFilesystem::FileInfo* VirtualFilesystem::LoadSpecificFile(const char* specific_path, void* (*allocator)(std::size_t)) const {
	for (const VfsMount& mount : mMounts) {
		if (FileInfo* loaded_data = mount.MountImpl->LoadSubFile(specific_path, allocator, true)) {
			return loaded_data;
		}
	}

	return {};
}

std::optional<std::filesystem::path> VirtualFilesystem::GetFilePath(const std::filesystem::path& path) const {
	for (const VfsMount& mount : mMounts) {
		if (auto file_path = mount.MountImpl->GetFilePath(path)) {
			return file_path;
		}
	}

	return {};
}

std::vector<std::filesystem::path> VirtualFilesystem::GetAllFilePaths(const std::filesystem::path& path) const {
	std::vector<std::filesystem::path> file_paths;

	for (const VfsMount& mount : mMounts) {
		if (auto file_path = mount.MountImpl->GetFilePath(path)) {
			file_paths.push_back(std::move(file_path).value());
		}
	}

	return file_paths;
}
