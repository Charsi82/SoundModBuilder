// Sound Mod Builder

#include "utf.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <shlwapi.h>
#include <string>
#include "data.h"
namespace fs = std::filesystem;

const char* XML_CAPTION = "<?xml version=\"1.0\" encoding=\"utf-8\"?>";

static std::wstring str2wstr(const std::string& str)
{
	return StringFromUTF8(str.c_str());
}

static void print_string(const std::string& str)
{
	std::cout << str << std::endl;
}

namespace Tags
{
	const char* TAG_GAME_PATH = "GAME_PATH";
	const char* TAG_SOURCE_DIR = "SOURCE_DIRECTORY";
	const char* TAG_MODCAPTION = "MOD_CAPTION";
	const char* TAG_MODDIRECTORY = "MOD_DIRECTORY";
	const char* TAG_MOD_PREFIX = "MOD_PREFIX";
	const char* TAG_WwiseCLI_PATH = "WwiseCLIPath";
	const char* TAG_PROJECT_PATH = "ProjectPath";
}

class COptions
{
	std::map<std::string, std::string> m_map
	{
		{ Tags::TAG_GAME_PATH, ""},
		{ Tags::TAG_SOURCE_DIR, ""},
		{ Tags::TAG_MODCAPTION, ""},
		{ Tags::TAG_MODDIRECTORY, ""},
		{ Tags::TAG_MOD_PREFIX, ""},
		{ Tags::TAG_WwiseCLI_PATH, ""},
		{ Tags::TAG_PROJECT_PATH, ""},
	};

	void read_option(std::string_view line)
	{
		for (auto& [k1, v1] : m_map)
		{
			if (line.starts_with(k1))
			{
				line.remove_prefix(k1.size() + 1);
				v1 = std::string(line);
				break;
			}
		}
	}

	bool check_options()
	{
		bool res = true;
		for (auto const& [k1, v1] : m_map)
		{
			if (!v1.size())
			{
				std::cout << "Параметр " << k1 << " задан неверно:" << std::endl;
				res = false;
			}
		}
		if (!fs::exists(fs::path(get_game_path())))
		{
			std::cout << std::format("Путь к игре не найден '{}'.", get_game_path()) << std::endl;
			return false;
		}
		if (!fs::exists(fs::path(get_src_path())))
		{
			std::cout << std::format("Не найден путь {} '{}'.", Tags::TAG_SOURCE_DIR, get_src_path()) << std::endl;
			return false;
		}
		if (!fs::exists(fs::path(get_wwise_path())))
		{
			std::cout << std::format("Не найден путь к WwiseCLI.exe '{}'.", get_wwise_path()) << std::endl;
			return false;
		}
		if (!fs::exists(fs::path(get_project_path())))
		{
			std::cout << std::format("Не найден путь к проекту Wwise2019 '{}'.", get_project_path()) << std::endl;
			return false;
		}

		const std::string prefix = get_mod_prefix();
		if (!prefix.ends_with("_"))	m_map[Tags::TAG_MOD_PREFIX] = prefix + "_";
		return res;
	}

public:
	COptions() = default;

	std::string get_game_path() const
	{
		return m_map.at(Tags::TAG_GAME_PATH);
	}
	std::string get_src_path() const
	{
		return m_map.at(Tags::TAG_SOURCE_DIR);
	}
	std::string get_wwise_path() const
	{
		return m_map.at(Tags::TAG_WwiseCLI_PATH);
	}
	std::string get_project_path() const
	{
		return m_map.at(Tags::TAG_PROJECT_PATH);
	}
	std::string get_mod_name() const
	{
		return m_map.at(Tags::TAG_MODCAPTION);
	}
	std::string get_mod_prefix() const
	{
		return m_map.at(Tags::TAG_MOD_PREFIX);
	}
	std::string get_mod_dir() const
	{
		return m_map.at(Tags::TAG_MODDIRECTORY);
	}

	bool read()
	{
		const char* CONFIG_INI = "config.ini";
		std::ifstream in(CONFIG_INI);
		if (!in.is_open())
		{
			print_string(std::format("Не могу открыть файл '{}'", CONFIG_INI));
			return false;
		}

		std::string line{};
		while (std::getline(in, line)) read_option(line);
		in.close();
		dump();
		return check_options();
	}

private:
	void dump() const
	{
#ifdef _DEBUG
		std::cout << "dump options:" << std::endl;
		for (auto& [k1, v1] : m_map)
		{
			std::cout << "option: " << k1 << ":" << v1 << std::endl;
		}
#endif
	}
};

class CTagWriter
{
	std::ofstream& m_out;
	std::string m_tag;
	int m_tabs;

public:
	CTagWriter(std::ofstream& out, std::string tagname, int tabs = 0) :
		m_out(out), m_tag(tagname), m_tabs(tabs)
	{
		write_indent();
		m_out << "<" << m_tag << ">\n";
	}
	~CTagWriter()
	{
		write_indent();
		m_out << "</" << m_tag << ">\n";
	}
	int indent() const { return m_tabs; }
	void add(std::string_view key, std::string_view val)
	{
		++m_tabs;
		write_indent();
		m_out << "<" << key << ">" << val << "</" << key << ">\n";
		--m_tabs;
	}

private:
	void write_indent()
	{
		for (int i = 0; i < m_tabs; ++i) m_out << '\t';
	}
};

class CState
{
	std::string m_name;
	std::string m_value;
public:
	CState(const std::string& name, const std::string& value) : m_name(name), m_value(value) {};
	void write(std::ofstream& out, int indent) const
	{
		CTagWriter writer(out, "State", indent);
		writer.add("Name", m_name);
		writer.add("Value", m_value);
	}
};

class CFile
{
	std::string m_name;

public:
	explicit CFile(const std::string& name) : m_name(name) {};
	void write(std::ofstream& out, int indent) const
	{
		CTagWriter writer(out, "File", indent);
		writer.add("Name", m_name);
	}
};

class CCondList
{
public:
	std::string prefix;
	std::vector<CState> m_slist;
	std::vector<CFile> m_flist;
	CCondList() = default;
	void write(std::ofstream& out, int indent) const
	{
		if (m_flist.size())
		{
			CTagWriter writer(out, "Path", indent);
			if (m_slist.size())
			{
				CTagWriter writer2(out, "StateList", indent + 1);
				for (const auto& e : m_slist)
					e.write(out, indent + 2);
			}
			else
			{
				for (int i = 0; i < indent + 1; ++i) out << '\t';
				out << "<StateList/>\n";
			}
			CTagWriter writer3(out, "FilesList", indent + 1);
			for (const auto& e : m_flist)
				e.write(out, indent + 2);
		}
	}
};

struct CFileName
{
	bool used = false;
	std::string name;
	CFileName(const std::string& fn) : name(fn) {};
};

class CEvtData
{
	std::string m_name{};
	std::string m_extid{};

public:
	std::vector<CCondList> m_list;
	explicit CEvtData(std::string&& name) : m_name(name) {};
	void set_extid(const std::string& extid) { m_extid = extid; }
	void write(std::ofstream& out, int indent) const
	{
		if (!empty())
		{
			CTagWriter writer(out, "ExternalEvent", indent);
			writer.add("Name", m_name);
			CTagWriter writer2(out, "Container", indent + 1);
			writer2.add("ExternalId", m_extid);
			writer2.add("Name", "Voice");
			for (const auto& e : m_list)
				e.write(out, indent + 2);
		}
	}

	void add_fn(const std::string_view mod_prefix, std::vector<CFileName>& fn_list)
	{
		for (auto& e : m_list)
			for (auto& fn : fn_list)
				if (fn.name.starts_with(mod_prefix))
				{
					std::string_view fn2(fn.name);
					fn2.remove_prefix(mod_prefix.size());
					if (fn2.starts_with(e.prefix))
					{
						fn2.remove_prefix(e.prefix.size());
						fn2.remove_suffix(4);
						while (fn2.size() && (fn2.at(0) == '_' || std::isdigit(fn2.at(0))))
							fn2.remove_prefix(1);
						if (fn2.empty() || fn2 == "fx")
						{
							fn.used = true;
							e.m_flist.push_back(CFile(fn.name));
						}
					}
				}
	}

	bool check_fn(std::string_view fn) const
	{
		return std::any_of(m_list.begin(), m_list.end(), [&fn](const auto& e) { return fn.find(e.prefix) != std::string::npos; });
	}

private:
	bool empty() const
	{
		return std::all_of(m_list.begin(), m_list.end(), [](const auto& e) { return e.m_flist.empty(); });
	}
};

class CModXml
{
	std::string m_modname;
	std::vector<CEvtData> m_events;
	std::vector<CFileName> list_fn;

public:
	explicit CModXml(const std::string& name) : m_modname(name) {};
	bool init()
	{
		std::stringstream data_ini{ data };
		std::string line;
		while (std::getline(data_ini, line))
		{
			if (line.starts_with("n"))
			{
				m_events.push_back(CEvtData(line.substr(1)));
			}
			else if (line.starts_with("e"))
			{
				auto& e = m_events.back();
				e.set_extid(line.substr(1));
			}
			else if (line.starts_with("s"))
			{
				auto& e = m_events.back();
				e.m_list.push_back(CCondList());
				auto& cl = e.m_list.back();
				std::stringstream tmp(line.substr(1));
				std::getline(tmp, cl.prefix, ',');
				while (tmp.good())
				{
					std::string name;
					std::getline(tmp, name, ',');
					std::string value;
					std::getline(tmp, value, ',');
					cl.m_slist.emplace_back(name, value);
				}
			}
		}
		return true;
	}

	bool write(const std::string& prefix, const std::string& path)
	{
		std::ofstream file(path + "\\mod.xml");
		if (!file.is_open())
		{
			print_string(std::format("Не могу записать файл: {}\\mod.xm", path));
			return false;
		}
		fill_flist(prefix, path);
		file << XML_CAPTION << std::endl;
		CTagWriter write1(file, "AudioModification.xml");
		CTagWriter write2(file, "AudioModification", write1.indent() + 1);
		write2.add("Name", m_modname);
		for (const auto& evt : m_events)
			evt.write(file, write2.indent() + 1);
		return true;
	}

	void print_unused()
	{
		list_fn.erase(
			std::remove_if(
				list_fn.begin(),
				list_fn.end(),
				[](auto& item) { return item.used; }),
			list_fn.end());
		if (list_fn.size())
		{
			print_string("*** Не используемые модом файлы:");
			for (const auto& s : list_fn)
			{
				std::cout << s.name << std::endl;
			}
		}
	}

	bool check_fn(std::string_view fn)
	{
		return std::any_of(m_events.begin(), m_events.end(), [&fn](const auto& e) { return e.check_fn(fn); });
	}

private:
	void fill_flist(std::string_view mod_prefix, std::string_view path)
	{
		for (auto const& dir_entry : fs::directory_iterator(fs::path(path)))
		{
			const std::string fn = dir_entry.path().filename().string();
			if (fn.ends_with(".wem"))
				list_fn.push_back(CFileName(fn));
		}
		for (auto& e : m_events)
			e.add_fn(mod_prefix, list_fn);
	}
};

std::string get_game_version(std::string path)
{
	std::ifstream pref(path);
	if (pref.is_open())
	{
		std::string line;
		while (std::getline(pref, line))
			if (line.find("last_server_version") != std::string::npos)
			{
				const size_t dotpos = line.rfind(',') + 1;
				const size_t lastpos = line.rfind('\t');
				return line.substr(dotpos, lastpos - dotpos);
			}
	}
	print_string(std::format("Не могу прочитать файл '{}'", path));
	return {};
}

int quit(const int ret)
{
	system("pause");
	return ret;
}

int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "ru_RU.UTF-8");
	print_string("*** Чтение настроек ***");
	COptions opts;
	if (!opts.read()) return quit(1);

	std::string game_ver = get_game_version(opts.get_game_path() + "/preferences.xml");
	if (!game_ver.size())
	{
		print_string("Версия игры не определена");
		return quit(1);
	}
	print_string(std::format("*** Версия игры: {} ***", game_ver));

	CModXml mod_xml(opts.get_mod_name());
	if (!mod_xml.init()) return quit(1);

	print_string("*** Переименование исходных файлов ***");
	for (auto const& dir_entry : fs::directory_iterator(fs::path(opts.get_src_path())))
	{
		const std::string fn = dir_entry.path().filename().string();
		if (!fn.starts_with(opts.get_mod_prefix()) && fn.ends_with(".wav") && mod_xml.check_fn(fn))
		{
			fs::path target_path(opts.get_src_path());
			target_path /= (opts.get_mod_prefix() + fn);

#ifdef _DEBUG
			std::cout << std::format("* Rename from '{}'\n*  to '{}'",
				dir_entry.path().filename().string(),
				target_path.filename().string()
			) << std::endl;
#endif // _DEBUG

			try
			{
				fs::rename(dir_entry.path(), target_path);
			}
			catch (fs::filesystem_error& e)
			{
				print_string(ConvertToUTF8(e.what(), 1251));
			}
		}
	}

	const fs::path tmp_dir = fs::path(opts.get_src_path()) / "Windows";

	print_string("*** Построение списка исходных файлов ***");
	std::vector<std::string> conv_files;
	for (auto const& dir_entry : fs::directory_iterator(fs::path(opts.get_src_path())))
	{
		const std::string fn = dir_entry.path().filename().string();
		if (fn.starts_with(opts.get_mod_prefix()) && fn.ends_with(".wav") && mod_xml.check_fn(fn))
		{
			conv_files.push_back(fn);
		}
	}

	const fs::path target_dir = fs::path(opts.get_game_path()) / "bin" / game_ver / "res_mods\\banks\\mods" / opts.get_mod_dir();
	fs::create_directories(target_dir);

	if (conv_files.size())
	{
		fs::create_directory(tmp_dir);
		const fs::path list_files = fs::path(tmp_dir) / "MySources.xml";
		std::ofstream f(list_files);

		f << XML_CAPTION << std::endl;
		f << "<ExternalSourcesList SchemaVersion=\"1\" Root=\"" << opts.get_src_path() << "\">" << std::endl;

		for (auto const& fn : conv_files)
			f << "	<Source Path=\"" << fn << "\" Conversion=\"WOWS_WEM_CONVERSION\"/>" << std::endl;

		f << "</ExternalSourcesList>" << std::endl;
		f.close();

		print_string("*** Запуск конвертации ***");
		const std::wstring wwp = str2wstr(opts.get_wwise_path());
		const std::wstring params = str2wstr(
			std::format("\"{}\" -ConvertExternalSources \"{}\" -ExternalSourcesOutput \"{}\"  -verbose",
				opts.get_project_path(),
				list_files.string(),
				opts.get_src_path()));

		SHELLEXECUTEINFOW ShRun{};
		ShRun.cbSize = sizeof(SHELLEXECUTEINFOW);
		ShRun.fMask = SEE_MASK_FLAG_NO_UI |
			SEE_MASK_NO_CONSOLE |
			SEE_MASK_FLAG_DDEWAIT | SEE_MASK_NOCLOSEPROCESS;
		ShRun.lpFile = wwp.c_str();
		ShRun.lpParameters = params.c_str();
		ShRun.nShow = SW_HIDE;

		if (!ShellExecuteExW(&ShRun) || !ShRun.hProcess)
		{
			print_string("Не могу запустить WwiseCLI.exe!");
			return quit(1);
		}
		WaitForSingleObject(ShRun.hProcess, INFINITE);

		DWORD exitCode = 0;
		if (::GetExitCodeProcess(ShRun.hProcess, &exitCode) == FALSE)
		{
			return quit(GetLastError());
		}

		print_string("*** Копируем файлы в папку мода ***");
		//int f_cnt = 0;
		for (auto const& dir_entry : fs::directory_iterator(tmp_dir))
		{
			const fs::path fpath = dir_entry.path();
			if (fpath.string().ends_with(".wem"))
			{
				try
				{
					fs::copy_file(fpath, target_dir / fpath.filename(), fs::copy_options::overwrite_existing);
					//++f_cnt;
				}
				catch (fs::filesystem_error& e)
				{
					print_string(ConvertToUTF8(e.what(), 1251));
				}
			}
		}
		//print_string(std::format("*** Скопировано файлов: {} ***", f_cnt));
	}

	print_string("*** Создание mod.xml ***");
	if (!mod_xml.write(opts.get_mod_prefix(), target_dir.string())) return quit(1);
	print_string("*** Файл mod.xml успешно сгенерирован ***");

	mod_xml.print_unused();

#ifndef _DEBUG
	print_string("*** Удаление временных файлов ***");
	try
	{
		fs::remove_all(tmp_dir);
	}
	catch (fs::filesystem_error& e)
	{
		print_string(ConvertToUTF8(e.what(), 1251));
	}
#endif

	print_string("*** Готово ***");
	return quit(0);
}
