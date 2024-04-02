*** This file is designed to desribe the implmentations of two additional features as required in the design specification document ***

1. User Registration
  On the client side, a user may create an account by using the following commnad:
    /create <username> <password> <server-IP> <server-port>
  If no account associated with the username has ever been created, the user will be logged onto the server and their username and password
are stored and kept permananetly in the database. This ensures all acounts created are permanent.
  The command works as follows:
    1. client end checks the command type
    2. matches "/create" 
    3. check constraints like the length of username etc.
    4. sends a registration signal to the server
    5. server checks if a user with the name has been created
      5.1 If positive, the request is denied and a REG_NAK signal will be sent back, indicating the failure to create an account
      5.2 If negative, the request is accepted and a REG_ACK signal will be sent back, indicating the success to create an account
    6. If an account is created successfully, the username and pssword will be stored in the database for future logins

2. Session Admins
  When a user creates a session successfully on the server side, they become the admin of the session. A session admin can remove a user from the session once a time. On top of this, an admin can promote someone in the session to admin and the promoter will lose their admin status. When an admin leaves the session and there is only one user remaining in the session, they are selected as the admin.
  On the client side, an admin may kick a user by using the following commnad:
    /kick <username>
  This operation may fail for following reasons:
    1. The request sender is not an admin
    2. User does not exist
    3. User is not in the session
    4. User is not in session
    5. User is not Online
  The command works as follows:
    1. client end checks the command type
    2. matches "/kick" 
    3. checks if the user is an admin and is in that session. If not, stop here and return an error message
    3. check constraints like the length of username etc.
    4. sends a Kick signal to the server
    5. server checks if the user is in the session. skip to step 9
    6  server checks if the user is in a different session. If yes, return the corresponding error message
    7. server checks if the user is online. If yes, return the corresponding error message
    8. server checks if the user with the name exists. If yes, return the corresponding error message
    9. The user in that session is removed by the server and does not belong in any session now
  
  On the client side, an admin may promote a user by using the following commnad:
    /promote <username>
  This operation may fail for following reasons:
    1. The request sender is not an admin
    2. User does not exist
    3. User is not in the session
    4. User is not in session
    5. User is not online
  The command works as follows:
    1. client end checks the command type
    2. matches "/promote" 
    3. checks if the user is an admin and is in that session. If not, stop here and return an error message
    3. check constraints like the length of username etc.
    4. sends a PROMOTE signal to the server
    5. server checks if the user is in the session. skip to step 9
    6  server checks if the user is in a different session. If yes, return the corresponding error message
    7. server checks if the user is online. If yes, return the corresponding error message
    8. server checks if the user with the name exists. If yes, return the corresponding error message
    9. The user is promoted by the server and the promoter is de-moted to USER.

Special Case: If all users except one in a session leave a session, the one still in the session is promtoed to admin
    The server checks the number of users whenever there is a user exiting the session. When the usser count is 1,
that user wil be promoted as the admin

*** end of document ***
