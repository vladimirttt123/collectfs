# Splitted Big Files Collector

Simple fuse file system that supports collect big files splitted by parts in different places.

The implementation is very simple and can be slow on big number (thouthands) of files.

All files used for read only purposes.

## Get and Build
FUSE 3 should be installed before building.
```
git clone https://github.com/vladimirttt123/collectfs.git
cd collectfs
make
```
In order allow to use from fstab need to be installed.
```
sudo make install
```

In order to remove intalled from system
```
sudo make uninstall
```

## Usage 
From command line any user:
```
collectfs config.file mount_point
```
Or add to /etc/fstab (need to be installed)
```
/path/to/config.file     /mnt/collectfs.mountpoint       collectfs   ro,allow_other    0 0
```
then
```
mount /mnt/collectfs.mountpoint
```

## Config File
Empty lines and lines started from # are skipped.
Than it is line of file name and one or more lines with full paths to parts of the file.

### Limitations
* No unicode support
* No subfolders support
* Full path to files should be used
### Example 
Following config defines 2 files each splitted in 2 folders
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

## Troubleshooting
In order to help understand the problems with config file there is ".fsinfo" file.
It presents invisibly or can be added to config and will be present visibly.
To understand the problem always possble read the file
```
cat /mnt/collectfs.mountpoint/.fsinfo
```
First row of the file is the version and path to config file. 
Following rows contains all significant rows of config file preceeded by number. 
Before first listing of mount point this numbers could be
-1, but after first listing all numbers should be non negatives and represents file size.
All rows with negative values should be checked for validity.