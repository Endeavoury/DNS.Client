# Publishing

The GitHub Actions workflow in `.github/workflows/package.yml` builds, tests, and
creates a package for pull requests and pushes to `master`. A push to `master`
publishes a uniquely versioned `0.0.0-ci.<run-number>` prerelease to both feeds. A
version tag publishes the stable version to both GitHub Packages and nuget.org.

## Configure nuget.org publishing

This workflow uses NuGet Trusted Publishing, so no long-lived API key is stored in
GitHub. In nuget.org account settings, create a Trusted Publishing policy with:

- Repository owner: `Endeavoury`
- Repository: `DNS.Client`
- Workflow file: `package.yml`
- Environment: leave empty unless the workflow is later assigned one

The NuGet account profile name is passed to `NuGet/login@v1` from the optional GitHub
Actions repository variable `NUGET_USER`. If that variable is absent, the workflow
uses the GitHub repository owner (`Endeavoury`). Set `NUGET_USER` only if the NuGet
profile name differs. Trusted Publishing issues a short-lived API key through GitHub
OIDC immediately before the push; the publish job therefore requests `id-token: write`
and does not need a `NUGET_API_KEY` secret. See [NuGet Trusted Publishing](https://learn.microsoft.com/en-us/nuget/nuget-org/trusted-publishing).

To publish a release to GitHub Packages, push a semantic-version tag prefixed with
`v`:

```bash
git tag v1.0.0
git push origin v1.0.0
```

The workflow removes the leading `v`, creates `DNS.Client.1.0.0.nupkg`, and pushes it
to `https://nuget.pkg.github.com/RoyGerritse/index.json` and to
`https://api.nuget.org/v3/index.json`. Both feeds are published only by the tag-only
job; nuget.org authentication is keyless through Trusted Publishing.

Package versions are immutable. If a release needs correction, create a new tag with
a new version rather than moving or reusing an existing tag.

## Consume from GitHub Packages

Add the owner feed once, using a GitHub personal access token (classic) with
`read:packages`:

```bash
dotnet nuget add source \
  --username YOUR_GITHUB_USERNAME \
  --password YOUR_GITHUB_TOKEN \
  --store-password-in-clear-text \
  --name github-roygerritse \
  https://nuget.pkg.github.com/RoyGerritse/index.json

dotnet add package DNS.Client --source github-roygerritse
```

Do not commit a token-bearing NuGet configuration file.
