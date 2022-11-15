# Script to run checks on the read and write operations.
#
# Usage:
#     bash tests/test_write.sh <mounting_directory> <fs_image>


MOUNTING_POINT=${1:-mnt}
IMAGE=${2:-../resources/bb_fs.img}

make clean
make
echo "---- TEST: Mounting the $IMAGE file system on ${MOUNTING_POINT}"
echo "$IMAGE ./$(basename $IMAGE)"
cp -f $IMAGE ./$(basename $IMAGE)
./fat-fuse ./$(basename $IMAGE) ${MOUNTING_POINT}

mapfile -t files < <( ls ${MOUNTING_POINT} )

clean_and_exit () {
  echo "---- TEST: Exiting"
  sleep 1  # Make sure it finished mounting
  fusermount -u ${MOUNTING_POINT}
  rm ./$(basename $IMAGE)
  exit $1
}

if [ ${#files[@]} -eq 0 ]
then
  echo "-------- TEST FAILED: Empty image"
  clean_and_exit -1
fi

echo "-------- VFS content"
for item in ${files[*]}
do
    echo "---------------- $item"
done

if [ ${#files[@]} \> 1 ] || [ "${files[0]}" != "1984.TXT" ]
then
  echo "-------- TEST FAILED: Use the original image with only 1984.TXT"
  clean_and_exit -1
fi

NOW=$(date +%s)
TODAY=$(date '+%Y-%m-%d')
# There can be a time zone difference because date uses UTC
TOMORROW=$(date -d '+1 day' '+%Y-%m-%d')


echo ""
TEST_DESCRIPTION="Create files"
touch ${MOUNTING_POINT}/newfile
exit_code=$?
if [ $exit_code -eq 0 ]
then
  echo "-------- TEST PASSED: $TEST_DESCRIPTION"
else
  echo "-------- TEST FAILED: $TEST_DESCRIPTION"
  clean_and_exit -1
fi

echo ""
TEST_DESCRIPTION="Create files and update creation time"
touch ${MOUNTING_POINT}/newfile1
modified_time=$(date +%s -r ${MOUNTING_POINT}/newfile1)
if [ $((modified_time + 0)) -ge $((NOW + 0)) ]
then
  echo "-------- TEST PASSED: $TEST_DESCRIPTION"
else
  echo $NOW
  echo $modified_time
  echo "-------- TEST FAILED: $TEST_DESCRIPTION"
  clean_and_exit -1
fi

echo ""
TEST_DESCRIPTION="Write files with one cluster"
DATA="Enough data for cluster 1"
echo $DATA > ${MOUNTING_POINT}/newfile2
counts=$(grep "$DATA" ${MOUNTING_POINT}/newfile2 | wc -l)
if [ "$counts" == "1" ]
then
  echo "-------- TEST PASSED: $TEST_DESCRIPTION"
else
  echo "-------- TEST FAILED: $TEST_DESCRIPTION"
  clean_and_exit -1
fi

echo ""
TEST_DESCRIPTION="Write files with multiple clusters"
yes | head -n 1024 > ${MOUNTING_POINT}/newfile3
counts=$(grep "y" ${MOUNTING_POINT}/newfile3 | wc -l)
if [ "$counts" == "1024" ]
then
  echo "-------- TEST PASSED: $TEST_DESCRIPTION"
else
  echo "-------- TEST FAILED: $TEST_DESCRIPTION"
  clean_and_exit -1
fi

echo ""
TEST_DESCRIPTION="Write files in sub-directories"
DATA="Writing in sub dirs"
mkdir ${MOUNTING_POINT}/newdir
echo $DATA > ${MOUNTING_POINT}/newdir/newfile2
counts=$(grep "$DATA" ${MOUNTING_POINT}/newdir/newfile2 | wc -l)
if [ "$counts" == "1" ]
then
  echo "-------- TEST PASSED: $TEST_DESCRIPTION"
else
  echo "-------- TEST FAILED: $TEST_DESCRIPTION"
  clean_and_exit -1
fi

echo ""
TEST_DESCRIPTION="Write existing file that had one cluster"
DATA="Write existing file"
echo "lala" > ${MOUNTING_POINT}/newfile4
echo $DATA > ${MOUNTING_POINT}/newfile4
counts=$(grep "$DATA" ${MOUNTING_POINT}/newfile4 | wc -l)
if [ "$counts" == "1" ]
then
  echo "-------- TEST PASSED: $TEST_DESCRIPTION"
else
  echo "-------- TEST FAILED: $TEST_DESCRIPTION"
  clean_and_exit -1
fi

echo ""
TEST_DESCRIPTION="Write existing file that had multiple clusters"
yes | head -n 1024 > ${MOUNTING_POINT}/newfile5
DATA="Write existing file"
echo $DATA > ${MOUNTING_POINT}/newfile5
counts=$(grep "$DATA" ${MOUNTING_POINT}/newfile5 | wc -l)
if [ "$counts" == "1" ]
then
  echo "-------- TEST PASSED: $TEST_DESCRIPTION"
else
  echo "-------- TEST FAILED: $TEST_DESCRIPTION"
  clean_and_exit -1
fi

echo ""
TEST_DESCRIPTION="Truncate file that had multiple clusters"
yes | head -n 1024 > ${MOUNTING_POINT}/newfile6
truncate -s 10 ${MOUNTING_POINT}/newfile6
counts=$(grep "y" ${MOUNTING_POINT}/newfile6 | wc -l)
if [ "$counts" == "5" ]
then
  echo "-------- TEST PASSED: $TEST_DESCRIPTION"
else
  echo "-------- TEST FAILED: $TEST_DESCRIPTION"
  clean_and_exit -1
fi

echo ""
TEST_DESCRIPTION="Truncate file and set time correctly"
original_time=$(date +%s -r ${MOUNTING_POINT}/1984.TXT)
truncate -s 10 ${MOUNTING_POINT}/1984.TXT
modified_time=$(date +%s -r ${MOUNTING_POINT}/1984.TXT)
if [ $((modified_time + 0)) -gt $((original_time + 0)) ]
then
  echo "-------- TEST PASSED: $TEST_DESCRIPTION"
else
  echo "-------- TEST FAILED: $TEST_DESCRIPTION"
  clean_and_exit -1
fi

# Create new test directory to avoid filling up the dentries of /
mkdir ${MOUNTING_POINT}/testdir/
echo ""
TEST_DESCRIPTION="Delete file"
rm ${MOUNTING_POINT}/testdir/newfile4
ls -l ${MOUNTING_POINT}/testdir/newfile4 &> /dev/null
exit_code=$?
if [ $exit_code -eq 0 ]
then
  echo "-------- TEST FAILED: $TEST_DESCRIPTION"
  clean_and_exit -1
else
  echo "-------- TEST PASSED: $TEST_DESCRIPTION"
fi

echo ""
TEST_DESCRIPTION="Delete empty directory with rmdir"
mkdir ${MOUNTING_POINT}/testdir/dir4 
rmdir ${MOUNTING_POINT}/testdir/dir4 
ls -l ${MOUNTING_POINT}/testdir/dir4 &> /dev/null
exit_code=$?
if [ $exit_code -eq 0 ]
then
  echo "-------- TEST FAILED: $TEST_DESCRIPTION"
  clean_and_exit -1
else
  echo "-------- TEST PASSED: $TEST_DESCRIPTION"
fi

echo ""
TEST_DESCRIPTION="Delete directory with rm -r"
mkdir ${MOUNTING_POINT}/testdir/dir5 
rm -r ${MOUNTING_POINT}/testdir/dir5 
ls -l ${MOUNTING_POINT}/testdir/dir5 &> /dev/null
exit_code=$?
if [ $exit_code -eq 0 ]
then
  echo "-------- TEST FAILED: $TEST_DESCRIPTION"
  clean_and_exit -1
else
  echo "-------- TEST PASSED: $TEST_DESCRIPTION"
fi

echo ""
TEST_DESCRIPTION="File is still deleted after re-mounting"
echo "... Unmounting and remounting system" 
sleep 1
fusermount -u ${MOUNTING_POINT}
sleep 1  # Make sure it finished mounting
./fat-fuse ./$(basename $IMAGE) ${MOUNTING_POINT} &> /dev/null
sleep 3ls -l ${MOUNTING_POINT}/testdir/newfile4 &> /dev/null
exit_code=$?
if [ $exit_code -eq 0 ]
then
  echo "-------- TEST FAILED: $TEST_DESCRIPTION"
  clean_and_exit -1
else
  echo "-------- TEST PASSED: $TEST_DESCRIPTION"
fi

echo ""
TEST_DESCRIPTION="Directory is still deleted after re-mounting"
echo "... Unmounting and remounting system" 
sleep 1
fusermount -u ${MOUNTING_POINT}
sleep 1  # Make sure it finished mounting
./fat-fuse ./$(basename $IMAGE) ${MOUNTING_POINT} &> /dev/null
sleep 3
ls -l ${MOUNTING_POINT}/testdir/dir5 &> /dev/null
exit_code=$?
if [ $exit_code -eq 0 ]
then
  echo "-------- TEST FAILED: $TEST_DESCRIPTION"
  clean_and_exit -1
else
  echo "-------- TEST PASSED: $TEST_DESCRIPTION"
fi

echo ""
TEST_DESCRIPTION="Delete non empty directory with rmdir should fail"
mkdir ${MOUNTING_POINT}/testdir/dir6/
mkdir ${MOUNTING_POINT}/testdir/dir6/dir6 
rmdir ${MOUNTING_POINT}/testdir/dir6
ls -l ${MOUNTING_POINT}/testdir/dir6 &> /dev/null
exit_code=$?
if [ $exit_code -eq 1 ]  # ls finds dir6
then
  echo "-------- TEST FAILED: $TEST_DESCRIPTION"
  clean_and_exit -1
else
  echo "-------- TEST PASSED: $TEST_DESCRIPTION"
fi

echo ""
TEST_DESCRIPTION="Delete non empty directory with rm -r should not fail" 
mkdir ${MOUNTING_POINT}/testdir/dir7/
touch ${MOUNTING_POINT}/testdir/dir7/newfile1
rm -r ${MOUNTING_POINT}/testdir/dir7
ls -l ${MOUNTING_POINT}/testdir/dir7 &> /dev/null
exit_code=$?
if [ $exit_code -eq 0 ]  # ls does not find dir7
then
  echo "-------- TEST FAILED: $TEST_DESCRIPTION"
  clean_and_exit -1
else
  echo "-------- TEST PASSED: $TEST_DESCRIPTION"
fi

echo ""
TEST_DESCRIPTION="Delete directory that is not last entry without unmounting"
mkdir ${MOUNTING_POINT}/testdir/dir9
mkdir ${MOUNTING_POINT}/testdir/dir9/dir1
mkdir ${MOUNTING_POINT}/testdir/dir9/dir2 
rmdir ${MOUNTING_POINT}/testdir/dir9/dir1 
ls -l ${MOUNTING_POINT}/testdir/dir9/dir1 &> /dev/null
exit_code=$?
if [ $exit_code -eq 0 ]
then
  echo "-------- TEST FAILED: $TEST_DESCRIPTION"
  clean_and_exit -1
else
  echo "-------- TEST PASSED: $TEST_DESCRIPTION"
fi

echo ""
TEST_DESCRIPTION="Delete file that is not last entry with unmounting"
mkdir ${MOUNTING_POINT}/testdir/dir10
touch ${MOUNTING_POINT}/testdir/dir10/file11
touch ${MOUNTING_POINT}/testdir/dir10/file12
# We don't delete file12, only file11
rm ${MOUNTING_POINT}/testdir/dir10/file11
touch ${MOUNTING_POINT}/testdir/dir10/file13
echo "... Unmounting and remounting system" 
sleep 1
fusermount -u ${MOUNTING_POINT}
sleep 1  # Make sure it finished mounting
./fat-fuse ./$(basename $IMAGE) ${MOUNTING_POINT} &> /dev/null
sleep 3
# file12 should still be there
ls -l ${MOUNTING_POINT}/testdir &> /dev/null
ls -l ${MOUNTING_POINT}/testdir/dir10 &> /dev/null
ls -l ${MOUNTING_POINT}/testdir/dir10/file12 &> /dev/null
exit_code=$?
if [ $exit_code -eq 0 ]
then
  echo "-------- TEST PASSED: $TEST_DESCRIPTION"
else
  echo "-------- TEST FAILED: $TEST_DESCRIPTION"
  clean_and_exit -1
fi


echo "---- TEST: All test TEST PASSED"

clean_and_exit 0