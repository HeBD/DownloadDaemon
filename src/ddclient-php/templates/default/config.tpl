<center><a href="index.php?site=config">General Configuration</a> | <a href="index.php?site=conf_reconnect">Reconnect Setup</a> | <a href="index.php?site=conf_premium">Premium account setup</a></center>

<form action="index.php?site=config" method="post">
	<!--fieldset style="width: 47%; float: left;"-->
	<fieldset>
		<legend>Change Password:</legend>
		<label for="opwd">Old Password:</label><input type="text" name="old_pw" id="opwd" /><br />
		<label for="npwd">New Password:</label><input type="text" name="new_pw" id="npwd" /><br />
		<label for="submit"> </label><input type="submit" name="change_pw" value="Change Password" id="submit" />
	</fieldset>
	<!--fieldset style="width: 47%; float: right;">
		<legend>Enable / Disable</legend>
		<label>test</label>
	</fieldset-->
	<fieldset>
		<legend>General Configuration</legend>
		You can force DownloadDaemon to only download at specific times. Therefore you can specifie a start time and an end time in the format hours:minutes.<br />
		Leave this fields empty if you want to allow DownloadDaemon to download permanently.<br>
		<label for="start">Start Time:</label><input type="text" name="download_timing_start" value="{DL_Start}" id="start" /><br />
		<label for="end">End Time:</label><input type="text" name="download_timing_end" value="{DL_End}" id="end" /><br />

		This option specifies where finished downloads should be safed on the server:<br />
		<label for="dl_folder">Download folder:</label><input type="text" size="40" name="download_folder" value="{DL_Folder}" id="dl_folder" /><br />

		Here you can specify how many downloads may run at the same time:<br />
		<label for="sim">Simultaneous downloads:</label><input type="text" size="5" name="simultaneous_downloads" value="{DL_sim}" id="sim" /><br />

		This option specifies DownloadDaemons logging activity.<br />
		<label for="debug">Debug Level:</label>{Debug}<br />

		Max download speed (will only be used for Downloads that are not yet running):<br />
		<label for="max_speed"> </label><input type="text" name="max_dl_speed" value="{Max_DL_Speed}" id="max_speed" /><br />

		<label for="submit"> </label><input type="submit" name="apply" value="Apply" id="submit" />
	</fieldset>
	<!--fieldset>
		<legend>Reconnect Configuration</legend>
		This option specifies DownloadDaemons reconnect activity: <br />
		HARD cancels all downloads if a reconnect is needed<br />
		CONTINUE only cancels downloads that can be continued after the reconnect<br>
		SOFT will wait until all other downloads are finished<br>
		PUSSY will only reconnect if there is no other choice (no other download can be started without a reconnect)<br>
		<label for="r_pol">{L_Reconnect_Policy}:</label>{Reconnect_Policy}<br />

		<label for="r_model">Router Model:</label>{Router_Model_List}<br />
		<label for="r_ip">Router IP:</label><input type="text" name="router_ip" value="{Router_IP}" id="r_ip" /><br />
		<label for="r_user">Username:</label><input type="text" name="router_username" value="{Router_User}" id="r_user" /><br />
		<label for="r_pwd">Password:</label><input type="text" name="router_password" id="r_pwd" /><br />
		<label for="submit"> </label><input type="submit" name="apply" value="Apply" id="submit" />
	</fieldset>
	<fieldset>
		<legend>Premium Account Configuration</legend>

	</fieldset-->
</form>
