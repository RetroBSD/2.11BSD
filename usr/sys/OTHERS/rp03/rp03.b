===============================================================================
Subject: bsd 2.9 boot problems with rp03 disk

I am running an 11/73 with rp03-emulating disks under 2.9.

My problem was that the l.s file doesn't know about rp03's
as root devices, and the kernel doesn't know what to do with the first
interrupt from the root device, or something similar.
This was despite the fact that the installation document
claims that 2.9 supports rp's as the root device.

It was easy to fix -- I just added another #if - #endif case
for rp at vector 254 in the l.s code.  After this, our
compiled systems booted without problem.

Also:  I found that the bootstrap code (marked UNTESTED) in
boot.s won't work, so we couldn't do AUTOBOOTS until 
I replaced that section with the code listed in the manual
page boot(8).
===============================================================================
