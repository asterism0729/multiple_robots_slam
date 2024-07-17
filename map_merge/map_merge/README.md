# map_merge

## This package is used to launch map_merge nodes

### "robot_name" require robot-number (ex. robot1, robot2, etc.)
# これを行うことで実行中に取れるトピックが増える可能性あり.同時にやっておいた方がいい
* merging maps

```
$ roslaunch map_merge map_merge.launch robot_name:=robot1
```
