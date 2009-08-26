#ifndef CFGFILE_H_
#define CFGFILE_H_

#include<fstream>
#include "../mt_string/mt_string.h"
#include<boost/thread.hpp>

class cfgfile {
public:
	/** Default constructor: Constructs the Object with default values:
 	* Token for comments: #
 	* Read Only file
 	* assignment token: =
 	*/
	cfgfile();

	/** Constructs the object and opens a config file. The User can specify if the file should be
	*  opened with write-access. Comment token is #
	*	@param filepath Path to the config-file
	*	@param is_writeable specifies rw/ro
	*/
	cfgfile(mt_string &filepath, bool is_writeable);

	/** The full constructor with every initializable element.
 	*	@param filepath Path to the config-file
 	*	@param comment_token Token used for comments
 	*	@param eqtoken Token used for assignemnts
 	*	@param is_writeable specifies rw/ro
 	*/
	cfgfile(mt_string &filepath, mt_string &comment_token, char eqtoken, bool is_writeable);

	/** Destructor - closes the file */
	~cfgfile();

	/** Opens the cfgfile, if the object was constructed without specifying one.
 	*  Also, the file can be changed, if there is already one opened.
 	*	@param filepath Path to the config-file
 	*	@param writeable specifies rw/ro
 	*/
	void open_cfg_file(const mt_string &filepath, bool writeable);


	/** Returns a configuration options found in a file.
 	*	@param cfg_identifier specifies the configruation option to search for
 	*	@returns Configuration value, or, if none found or no file is opened, an empty string
 	*/
	mt_string get_cfg_value(const mt_string &cfg_identifier);

	/** Sets a configuration option. If the identifier already exists, the value is changed. Else, the option is appended.
 	*	@param cfg_identifier Identifier to search for/add
 	*	@param cfg_value the value that should be set
 	*  @returns true on success, else false
 	*/
	bool set_cfg_value(const mt_string &cfg_identifier, const mt_string &cfg_value);

	/** Get comment token
 	*	@returns current comment token
 	*/
	mt_string get_comment_token() const;

	/** Sets the comment token that should be used
 	*	@param comment_token Token to set
 	*/
	void set_comment_token(const mt_string &comment_token);

	/** reloads the file to make changes visible */
	void reload_file();

	/** closes the file */
	void close_cfg_file();

	/** Get writeable status
 	*	@returns True if writeable, false if RO
 	*/
	bool writeable() const;

	/** Get the filepath
	 *	@returns The Filepath to the config-file
	*/
	mt_string get_filepath() const;

	/** Get a config variable list
 	*	@param resultstr A string in which the result is stored line by line
 	*	@returns true on success
 	*/
	bool list_config(mt_string& resultstr);

private:
	/** Internal use: removes whitespaces from the beginning and end of a string.
	*  @param str String to change
	*/
	void trim(mt_string &str) const;
	std::fstream file;
	mt_string filepath;
	mt_string comment_token;
	bool is_writeable;
	char eqtoken;
	boost::mutex mx;

};

#endif /*CFGFILE_H_*/
