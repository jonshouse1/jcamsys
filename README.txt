All slightly incomplete and experimental, basic functionality works though.





Security
--------
   Basically the system has two entities, a "key" and a "password" - they relate to each
   other.


   For a first time install do the following (in order, the order does matter!).

(On the server)
	Step 1) "jcmakeserverpasswd"

		This will make a file "/etc/jcam_passwd" containing an MD5 hash of the 
		password. The server will pick this up when started.

	Step 2) "jcmakekey"

		This produces a key file "/etc/jcam_key".  This is your key and needs to
		be the same on all the clients.
		This is a block of random data described in ASCII hex. It is used by the 
		encryption between the server and its clients.

	Step 3) "jcmakeclientpasswd"

		Produces a file ".jcpass"
		This is an encrypted form the password (same password as "jcmakeserverpasswd")
		It needs the key file "/etc/jcam_key"
		Clients use the key to decrypt this password and present it to the server 
		when connecting.


	Step 4) Backup the 3 files "/etc/jcam_key" "/etc/jcam_passwd" ".jcpass" - you may need
		them again sometime.


	Step 5) On the client:
		The two files "/etc/jcam_key" and ".jcpass" need to be copied onto the client.
		".jcpass" can go in the users home directory, or if all else fails place it in
		root "/"



Examples (server):
	# jcmakeserverpasswd
	password: *********
	Created /etc/jcam_passwd

	# jcmakekey
	Created /etc/jcam_key

	# jcamclientpasswd
	password: *********
	Created .jcpass





