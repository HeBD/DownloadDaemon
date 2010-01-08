<form action="index.php?site=login" method="post">
	<fieldset>
		<legend>{L_Site}</legend>
		<label for="host">{L_Host}/{L_Port}:</label><input type="text" name="host" value="{C_DEFAULT_HOST}" id="host" /> <input type="text" name="port" value="{C_DEFAULT_PORT}" size="5" id="host"><br />
		<label for="pwd">{L_Password}:</label><input type="password" name="passwd" id="pwd" /><br />
		<label for="encrypt">{L_Encrypt}:</label><input type="checkbox" name="encrypt" value="1" id="encrypt" /><br />
		<label for="submit"> </label><input type="submit" name="submit" value="{L_Site}" id="submit" />
	</fieldset>
</form>	
