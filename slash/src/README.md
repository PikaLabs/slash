Slash

Rsync

slash/include/rsync.cc

Rsync module is designed to sync document between hosts.

In the destination host, a deamon is setup by calling slash::StartRsync. Meanwhile, rsync config file is created under "raw_path". 

In the source host, several slash::RsyncSendFile is invoked, it depends on the files you may need to sync. Then, if you need to delete the files in the remove folder that don't exist in the source host, slash::RsyncSendClearTarget is standby.

After RsyncSendFile process, in the destination host, slash::StopRsync will help you terminate rsync deamon and cleanup everything for you.