# Splitted Big Files Collector

Simple fuse file system that supports collect big files splitted by parts in different places.

The implementation is very simple and can be slow on big number (thouthands) of files.

All files used for read only purposes.

## Get and Build
FUSE 3.1 should be installed before building.
```
git clone https://github.com/vladimirttt123/collectfs.git
cd collectfs
make
```

## Usage 
```
collectfs config.file mount_point
```
## Config File
Empty lines and lines started from # are skipped.
Than it is line of file name and one or more lines with full paths to parts of the file.

For example following config defines 2 files each splitted in 2 folders
```
file1.txt
/mnt/disk1/file.txt.part1
/mnt/disk2/file.txt.part2

# This is a comment line
file2.txt
/mnt/disk3/file.txt.part1
/mnt/disk4/file.txt.part2

```

After mounting with such config file list of mount point should show 2 files with names file1.txt and file2.txt

# Warning! Supposed no referenced files changing during mount!
First create files than add it to config, than you can mount.