md tmp
copy "pokemon_yellow_installer\Comments.txt" "tmp\Comments.txt"
copy "pokemon_yellow_installer\Header.txt" "tmp\Header.txt"
copy "pokemon_yellow_installer\Input Log.txt" "tmp\Input Log.txt"
copy "pokemon_yellow_installer\Subtitles.txt" "tmp\Subtitles.txt"
copy "pokemon_yellow_installer\SyncSettings.json" "tmp\SyncSettings.json"

"code\inst"
"zip\zip" -JXjq9 fix94-pokemonyellow-installer.bk2 "tmp\Comments.txt" "tmp\Header.txt" "tmp\Input Log.txt" "tmp\Subtitles.txt" "tmp\SyncSettings.json"

del "tmp\Comments.txt"
del "tmp\Header.txt"
del "tmp\Input Log.txt"
del "tmp\Subtitles.txt"
del "tmp\SyncSettings.json"

copy "pokemon_yellow_updater\Comments.txt" "tmp\Comments.txt"
copy "pokemon_yellow_updater\Header.txt" "tmp\Header.txt"
copy "pokemon_yellow_updater\Input Log.txt" "tmp\Input Log.txt"
copy "pokemon_yellow_updater\Subtitles.txt" "tmp\Subtitles.txt"
copy "pokemon_yellow_updater\SyncSettings.json" "tmp\SyncSettings.json"

"code\inst"
"zip\zip" -JXjq9 fix94-pokemonyellow-updater.bk2 "tmp\Comments.txt" "tmp\Header.txt" "tmp\Input Log.txt" "tmp\Subtitles.txt" "tmp\SyncSettings.json"

del "tmp\Comments.txt"
del "tmp\Header.txt"
del "tmp\Input Log.txt"
del "tmp\Subtitles.txt"
del "tmp\SyncSettings.json"
rmdir tmp

pause