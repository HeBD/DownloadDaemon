rapidshare_com = {
	get_compatible_hosts = function()
		hosts = { ".*rapidshare.com.*" }
		return hosts;
	end;

	get_plugin_type = function()
		return "hoster"
	end;

	get_plugin_version = function()
		return 1
	end;
}



r = regex.RegEx()
r:compile(".*")
