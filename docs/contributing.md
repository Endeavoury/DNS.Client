# Contributing

## Local setup

Install the latest .NET SDK, clone the repository, and run:

```bash
dotnet restore DNS.Client.sln
dotnet build DNS.Client.sln --configuration Release
dotnet test DNS.Client.sln --configuration Release
```

The library targets `netstandard2.0` and `netstandard2.1`; tests and the sample use the current SDK.

## Making protocol changes

Add or update a focused codec test for every new record or wire rule. Test both
compressed and uncompressed names, malformed lengths, and a round trip through
`Parse(ToArray(message))`. Never trust RDLENGTH or compression pointers from input.

## Pull requests

Keep commits focused, document public APIs with XML comments, and update the relevant
guide under `docs/`. Pull requests run the build and test workflow. Pushing to
`master` creates the next patch release automatically, so use a feature branch for
normal development.
