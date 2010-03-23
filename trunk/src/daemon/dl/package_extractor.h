#ifndef PACKAGE_EXTRACTOR_H_INCLUDED
#define PACKAGE_EXTRACTOR_H_INCLUDED
#include <string>



class pkg_extractor {
public:
	enum extract_status { PKG_SUCCESS, PKG_PASSWORD, PKG_INVALID, PKG_ERROR };
	enum tool { GNU_UNRAR, RARLAB_UNRAR, TAR, TAR_BZ2, TAR_GZ, ZIP, NONE };


	static extract_status extract_package(const std::string& filename, const std::string& password = "");
	static tool required_tool(std::string filename);
	static tool get_unrar_type();

	static extract_status extract_rar(const std::string& filename, const std::string& password, tool t);
	static extract_status extract_tar(const std::string& filename, tool t);
	static extract_status extract_zip(const std::string& filename, const std::string& password, tool t);

};

#endif // PACKAGE_EXTRACTOR_H_INCLUDED
