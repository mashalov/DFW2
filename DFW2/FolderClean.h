#pragma once
#include <filesystem>
#include <functional>
#include <fstream>
#include <set>
#include "dfw2exception.h"
#include "Messages.h"

class CFolderClean
{

public:
	using fnReportT = std::function<void(const std::string_view&)>;
protected:
	fnReportT fnReport_;
    static constexpr size_t Unlimited_ = (std::numeric_limits<size_t>::max)();
	size_t AllowedFilesCount_ = CFolderClean::Unlimited_;
	size_t AllowedSizeInMbytes_ = CFolderClean::Unlimited_;
	std::filesystem::path path_;
	long CheckPeriodInSecs_ = 60;
    static size_t Unlimit(const size_t& Value)
    {
        return Value == 0 ? CFolderClean::Unlimited_: Value;
    }

	static bool IsUnlimited(const size_t& limit)
	{
		return limit == CFolderClean::Unlimited_;
	}


	void Report(const std::string_view& Message)
	{
		if (fnReport_)
			(fnReport_)(Message);
	}

public:

	CFolderClean(const std::filesystem::path& path,
		const size_t AllowedFilesCount,
		const size_t AllowedSizeInMbytes) :
		path_(path),
		AllowedFilesCount_(CFolderClean::Unlimit(AllowedFilesCount)),
		AllowedSizeInMbytes_(CFolderClean::Unlimit(AllowedSizeInMbytes)) 
    {
        if (path_.has_extension() && path_.has_filename())
            path_.remove_filename();
    };

    void SetCheckPerion(long CheckPeriodInSecs)
    {
        CheckPeriodInSecs_ = CheckPeriodInSecs;
    }

	void SetReportFunction(fnReportT fnReport)
	{
		fnReport_ = fnReport;
	}

    void Clean()
    {
        // если фактически ограничений нет - просто выходим
        if (CFolderClean::IsUnlimited(AllowedFilesCount_) && CFolderClean::IsUnlimited(AllowedSizeInMbytes_))
            return;

        struct FileTime
        {
            const std::filesystem::path Path_;
            std::filesystem::file_time_type Time_;
            double Size_;
        };

        auto fnFileTimeCompare = [](const FileTime& lhs, const FileTime& rhs)
        {
            return std::tie(lhs.Time_, lhs.Size_, lhs.Path_) < std::tie(rhs.Time_, rhs.Size_, rhs.Path_);
        };

        // получаем текущее время
        const auto CurrentTime{ std::chrono::system_clock::now() };
        std::chrono::seconds CurrentTimeInSeconds{ std::chrono::duration_cast<std::chrono::seconds>(CurrentTime.time_since_epoch()) };
        std::chrono::seconds LastTimeInSeconds{ 0 };

        // пытаемся прочитать файл-маркер, в котором указано время последней очистки
        const auto MarkerFile{ path_ / "last_clean_marker" };
        std::ifstream  MarkerIn{ MarkerFile };
        if (MarkerIn.is_open())
        {
            // если файл есть, читаем время последней очистки
            MarkerIn.read(reinterpret_cast<char*>(&LastTimeInSeconds), sizeof(std::chrono::seconds));
            MarkerIn.close();
        }
        // если файла маркера нет - время последней очистки равно нулю
        // если с момента последней очистки прошло меньше заданного периода очистки ничего не делаем

        if (const auto SecondsLeft{ (CurrentTimeInSeconds - LastTimeInSeconds).count() - CheckPeriodInSecs_ }; SecondsLeft < 0)
        {
            Report(fmt::format(DFW2::CDFW2Messages::m_cszNextCacheClieanIn, stringutils::utf8_encode(path_.c_str()), -SecondsLeft));
            //pTree->Information();
            return;
        }

        // пишем в файл маркера текущее время
        std::ofstream MarkerOut{ MarkerFile };
        if (MarkerOut.is_open())
        {
            MarkerOut.write(reinterpret_cast<const char*>(&CurrentTimeInSeconds), sizeof(std::chrono::seconds));
            MarkerOut.close();
        }

        std::set<FileTime, decltype(fnFileTimeCompare)> files{ fnFileTimeCompare };
        std::error_code ec;

        // собираем файлы из каталога в сет, сортируем по дате, размеру и пути
        double CachedFilesSize{ 0.0 };
        for (const auto& file : std::filesystem::directory_iterator(path_))
            if (std::filesystem::is_regular_file(file, ec))
                // подсчитываем размер файлов в каталоге
                CachedFilesSize += files.insert({ file.path(),
                                                  std::filesystem::last_write_time(file, ec),
                                                  static_cast<double>(file.file_size(ec)) / 1024 / 1024
                    }).first->Size_;


        // считаем количество файлов для удаления
        size_t FilesToDelete{ files.size() > AllowedFilesCount_ ? files.size() - AllowedFilesCount_ : 0 };

        // если подсчитанные количество файлов и размер меньше, чем заданные ограничения - выходим
        if (FilesToDelete == 0 && AllowedSizeInMbytes_ >= CachedFilesSize)
            return;

        constexpr const char* cszInf{ "∞" };

        Report(fmt::format(DFW2::CDFW2Messages::m_cszTooManyCachedModules,
            files.size(),
            CachedFilesSize,
            CFolderClean::IsUnlimited(AllowedFilesCount_) ? cszInf : std::to_string(AllowedFilesCount_),
            CFolderClean::IsUnlimited(AllowedSizeInMbytes_) ? cszInf : std::to_string(AllowedSizeInMbytes_),
            stringutils::utf8_encode(path_.c_str())));

        //pTree->Information();

        for (const auto& file : files)
        {
            // работаем в цикле до тех пор, пока есть файлы для удаления
            // и размер файлов не снизился до ограничения
            if (FilesToDelete == 0 && CachedFilesSize < AllowedSizeInMbytes_)
                break;

            // не удаляем файл маркера
            if (file.Path_ == MarkerFile)
                continue;

            // удаляем файл
            std::filesystem::remove(file.Path_, ec);
            if (!ec)
            {
                Report(fmt::format(DFW2::CDFW2Messages::m_cszRemovingExcessCachedModule,
                    stringutils::utf8_encode(file.Path_.c_str())));
                //pTree->Information();
                // уменьшаем суммарный размер файлов на размер удаленного файла
                CachedFilesSize -= file.Size_;
                if (FilesToDelete > 0)
                    FilesToDelete--;
            }
        }
    }
};
