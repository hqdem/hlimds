# Valgrind README

In this guide the instruction on how to run Utopia EDA with Valgrind
and check memory leaks is described.
The guide was checked on Ubuntu 20.04 operating system.

## Valgrind installation

To install Valgrind, do the following:

```console
cd <workdir>
git clone https://sourceware.org/git/valgrind.git
cd valgrind
git checkout VALGRIND_3_23_0
cd coregrind/
export old_line1="   if (req_alignB > 16 \* 1024 \* 1024) {"
export new_line1="   if (req_alignB > 1024 * 1024 * 1024) {"
sed -i "s|$old_line1|$new_line1|" "./m_mallocfree.c"
export old_line2="                  a, req_alignB, req_pszB, req_alignB, 16 \* 1024 \* 1024 );"
export new_line2="                  a, req_alignB, req_pszB, req_alignB, 1024 * 1024 * 1024 );"
sed -i "s|$old_line2|$new_line2|" "./m_mallocfree.c"
cd ../
./autogen.sh
./configure
make -j$(nproc)
sudo make install
echo export DEBUGINFOD_URLS="https://debuginfod.archlinux.org" >> ~/.bashrc
sudo apt-get update
sudo apt install libc6-dbg
```

---

## Valgrind usage

To check program on memory leaks with Valgrind, do the following:

```console
valgrind ./program args
```

If Valgrind works, the output should be like:

```text
LEAK SUMMARY:
definitely lost: 0 bytes in 0 blocks
indirectly lost: 0 bytes in 0 blocks
possibly lost: 67,108,864 bytes in 1 blocks
still reachable: 67,108,864 bytes in 1 blocks
suppressed: 0 bytes in 0 blocks
Rerun with --leak-check=full to see details of leaked memory

ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
```

---

For more Valgrind options check [Valgrind manual](https://valgrind.org/docs/manual/manual-core.html#manual-core.options).
