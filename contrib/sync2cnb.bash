#! /bin/bash

cd `dirname "$0"`/..

rm -rf   /tmp/shynur/seer/taskrunner.git
mkdir -p /tmp/shynur/seer
cp -r .  /tmp/shynur/seer/taskrunner.git
cd       /tmp/shynur/seer/taskrunner.git

git remote set-url origin https://cnb.cool/seer-robotics/tools/TaskRunner.git
git push
