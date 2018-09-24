md tmp
copy "pokemon_gelb\Comments.txt" "tmp\Comments.txt"
copy "pokemon_gelb\Header.txt" "tmp\Header.txt"
copy "pokemon_gelb\Input Log.txt" "tmp\Input Log.txt"
copy "pokemon_gelb\Subtitles.txt" "tmp\Subtitles.txt"
copy "pokemon_gelb\SyncSettings.json" "tmp\SyncSettings.json"

"code\inst"
"zip\zip" -JXjq9 pokemon_gelb_installer.bk2 "tmp\Comments.txt" "tmp\Header.txt" "tmp\Input Log.txt" "tmp\Subtitles.txt" "tmp\SyncSettings.json"

del "tmp\Comments.txt"
del "tmp\Header.txt"
del "tmp\Input Log.txt"
del "tmp\Subtitles.txt"
del "tmp\SyncSettings.json"
rmdir tmp

pause