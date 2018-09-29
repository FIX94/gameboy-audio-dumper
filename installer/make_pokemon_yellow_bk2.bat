md tmp
copy "pokemon_yellow\Comments.txt" "tmp\Comments.txt"
copy "pokemon_yellow\Header.txt" "tmp\Header.txt"
copy "pokemon_yellow\Input Log.txt" "tmp\Input Log.txt"
copy "pokemon_yellow\Subtitles.txt" "tmp\Subtitles.txt"
copy "pokemon_yellow\SyncSettings.json" "tmp\SyncSettings.json"

"code\inst"
"zip\zip" -JXjq9 fix94-pokemonyellow-installer.bk2 "tmp\Comments.txt" "tmp\Header.txt" "tmp\Input Log.txt" "tmp\Subtitles.txt" "tmp\SyncSettings.json"

del "tmp\Comments.txt"
del "tmp\Header.txt"
del "tmp\Input Log.txt"
del "tmp\Subtitles.txt"
del "tmp\SyncSettings.json"
rmdir tmp

pause