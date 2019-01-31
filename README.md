# Networking-offline-4-Reliable_data_transfer

directly go to working_code/

rdt_gbn.c has gbn implementation without wrap around of seq number. so it isn't exactly gbn.

rdt_gbn_wrap.c is also ideally wrong since the sliding window concept is not implemented and the window doesn't slide, rather stays fixed.

rdt_gbn_2.c has cumulative ack implemented as part of bonus 2, which is wrongly done since all the ack packets were sent in B_timerinterrupt().
