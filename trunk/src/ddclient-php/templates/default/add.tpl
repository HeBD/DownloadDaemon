<form action="index.php?site=add" method="post">
	<fieldset>
		<legend>{L_Add_single_DL}:</legend>
		<label for="title">{L_Title}:</label><input type="text" name="title" id="title" /><br />
		<label for="url">{L_URL}:</label><input type="text" name="url" id="url" /><br />
		<label for="submit_single"> </label><input type="submit" value="{L_Add_single_DL}" name="submit_single" id="submit_single" />
	</fieldset>

	<fieldset>
		<legend>{L_Add_multi_DL}:</legend>
		<label for="urls">{L_URL}s:</label><textarea name="titles_urls" cols="120" rows="20" id="urls"></textarea><br />
		<label for="submit_multiple"> </label><input type="submit" value="{L_Add_multi_DL}" name="submit_multiple" id="submit_multiple" />
		{L_Add_multi_DL_Desc}
	</fieldset>
</form>
