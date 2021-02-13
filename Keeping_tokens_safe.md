# Keeping tokens safe

In the browser you need to be able to allow the user to refresh their window or keep their session alive even after a token has expired.  

* Access tokens are short-lived, depending on the application you might vary the lifetime, but IBM Cloud access tokens have a lifetime of 60 minutes.
* Refresh tokens have a longer lifetime and can be from days to a year.

## Why have a refresh token?

As the access token is short lived you may want the user to be able to walk away and then come back and continue using your application without logging back in.  The refresh token allows your application to get a new access token and continue a session without burdening the user with another login.  Refresh tokens are not always implemented and their use depends on the risk profile of your service.

## What are JWT Tokens?

[RFC 7519](https://tools.ietf.org/html/rfc7519) is the specification for JWT tokens.  This specifies the **FORMAT** of the access token.  A token is simply an encoded/hashed JSON format which contains 

> JSON Web Token (JWT) is a compact, URL-safe means of representing
   claims to be transferred between two parties.  The claims in a JWT
   are encoded as a JSON object that is used as the payload of a JSON
   Web Signature (JWS) structure or as the plaintext of a JSON Web
   Encryption (JWE) structure, enabling the claims to be digitally
   signed or integrity protected with a Message Authentication Code
   (MAC) and/or encrypted.


JWT Tokens are designed in such a way that the resource server does not need access to any database to verify that the token is valid.

## Setting the refresh token as a cookie

In a header you set the following:

```
Set-Cookie: refresh_token=<value>; expires=<unix timestamp a year in the future>; domain=<mydomain.com>; path=/; secure; HttpOnly
```

You can also set this as a Cookie via the [Javascript Cookie API](https://www.w3schools.com/js/js_cookies.asp).

## Attacks against OAuth2 Tokens

This is a quick summary of the Cotter reference below, which you should read.

* Replay attacks - sending same request twice, solution is to use nonce values
* Cross site request forgery (CSRF) - forces an end user to execute unwanted actions when already authenticated,  keep access token in memory and store refresh token as a cookie with httpOnly set. Assumes authentication server is using CORS policy correctly.
* Cross Site Scripting (XSS) - attackers can inject client-side scripts into pages viewed by other users, don't store access tokens in local storage

## Further Reading

* https://dev.to/cotter/what-on-earth-is-oauth-a-super-simple-intro-to-oauth-2-0-access-tokens-and-how-to-implement-it-in-your-site-48jo
* https://jwt.io/introduction
* https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Set-Cookie
* https://www.valentinog.com/blog/oauth2/