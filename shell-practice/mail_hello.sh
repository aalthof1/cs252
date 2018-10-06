#!/bin/bash
echo "Hello $USER!!" > temp-message
echo >> temp-message
echo "Today is" `date` >> temp-message
echo >> temp-message
echo "Sincerely," >> temp-message
echo " Myself" >> temp-message
/usr/bin/mailx -s "mail-hello" $USER < temp-message
echo "Message sent."
