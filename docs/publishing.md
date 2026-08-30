# Publishing

The workflows build and test pull requests. Each push to `master` creates a semantic
release and publishes a stable package to both GitHub Packages and nuget.org. The
first release is `1.0.0`; later commits increment only the patch component:
`1.0.1`, `1.0.2`, and so on.

To choose a new major or minor line yourself, create and push a tag such as `v2.0.0`
or `v1.1.0`. The tag is released immediately, and the next commit then continues
from that tag (`v2.0.1` or `v1.1.1`).

## Configure nuget.org publishing

This workflow uses NuGet Trusted Publishing, so no long-lived API key is stored in
GitHub. In nuget.org account settings, create a Trusted Publishing policy with:

- Repository owner: `Endeavoury`
- Repository: `DNS.Client`
- Workflow file: `package.yml`
- Environment: leave empty unless the workflow is later assigned one

The NuGet account profile name is passed to `NuGet/login@v1.2.0` from the optional GitHub
Actions repository variable `NUGET_USER`. If that variable is absent, the workflow
uses the linked NuGet profile `RoyGerritse`. Set `NUGET_USER` only if the NuGet
profile name differs. Trusted Publishing issues a short-lived API key through GitHub
OIDC immediately before the push; the publish job therefore requests `id-token: write`
and does not need a `NUGET_API_KEY` secret. See [NuGet Trusted Publishing](https://learn.microsoft.com/en-us/nuget/nuget-org/trusted-publishing).

To publish a release to GitHub Packages, push a semantic-version tag prefixed with
`v`:

```bash
git push origin master
```

For a manual major/minor baseline:

```bash
git tag v2.0.0
git push origin v2.0.0
```

The `master` release workflow will use that tag as the next version baseline.

The workflow creates a GitHub Release and `DNS.Client.1.0.0.nupkg`, then pushes it to
`https://nuget.pkg.github.com/RoyGerritse/index.json` and
`https://api.nuget.org/v3/index.json`. GitHub Packages authentication uses the
workflow token; nuget.org authentication is keyless through Trusted Publishing.

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
