#!/bin/sh
#uci show exception_post.email_post
#exception_post.email_post.smtp_server='smtp.qq.com:587'
#exception_post.email_post.smtp_ssl='1'
#exception_post.email_post.from_email='xxxxx@qq.com'
#exception_post.email_post.from_email_pwd='*******'
#exception_post.email_post.to_emai='xxxx@163.com'
#exception_post.email_post.enabled='0'

FLAG=`uci get exception_post.email_post.enabled`

if [ $FLAG -eq 0 ]; then
	echo "Email Not send, enable is false"
	#exit
fi

SMTP_SERVER=`uci get exception_post.email_post.smtp_server`
SMTP_USESSL=`uci get exception_post.email_post.smtp_ssl`
EMAIL_FROM=`uci get exception_post.email_post.from_email`
EMAIL_FROMPWD=`uci get exception_post.email_post.from_email_pwd`
EMAIL_TO=`uci get exception_post.email_post.to_emai`

echo "Start send email, from :$EMAIL_FROM to:$EMAIL_TO"

if [ ! -d /etc/ssmtp ]; then
	mkdir /etc/ssmtp
	touch /etc/ssmtp/ssmtp.conf
fi

echo "root=$EMAIL_FROM" > /etc/ssmtp/ssmtp.conf
echo "mailhub=$SMTP_SERVER" >> /etc/ssmtp/ssmtp.conf

if [ $SMTP_USESSL -eq 1 ]; then
	echo "UseTLS=YES" >> /etc/ssmtp/ssmtp.conf
	echo "UseSTARTTLS=YES" >> /etc/ssmtp/ssmtp.conf
fi

echo "FromLineOverride=YES" >> /etc/ssmtp/ssmtp.conf

( echo "From:<$EMAIL_FROM>";
echo "To:<$EMAIL_TO>";
echo "Subject:网关通知"
echo "Content-type: text/plain; charset=utf-8"
echo "";
echo "报警通知,用户帐号被锁"
echo ""
#read LINE
#while [ -n $LINE ]
#do
# echo $LINE
#read LINE
#done
)|ssmtp -v -v -au$EMAIL_FROM -ap$EMAIL_FROMPWD $EMAIL_TO


