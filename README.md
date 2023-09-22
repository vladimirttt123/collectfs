# collectfs

Simple fuse file system that supports collect big files splitted by parts in different places.

config files has followint format

filename
full_path_to_part1
full_path_to_part2
...

next_filename
full_path_to_part1_of_file2
full_path_to_part2_of_file2
...

Warning! Supposed no mentioned files change during mount!