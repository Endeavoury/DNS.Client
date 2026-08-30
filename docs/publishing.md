# Publishing

The GitHub Actions workflow in `.github/workflows/package.yml` builds, tests, and
creates a package for pull requests and pushes to `master`.

To publish a release to GitHub Packages, push a semantic-version tag prefixed with
`v`:

```bash
git tag v1.0.0
git push origin v1.0.0
```

The workflow removes the leading `v`, creates `DNS.Client.1.0.0.nupkg`, and pushes it
to `https://nuget.pkg.github.com/RoyGerritse/index.json`. Authentication uses the
workflow's short-lived `GITHUB_TOKEN`; no personal access token needs to be stored in
the repository. The workflow grants package write permission only to the tag-only
publish job.

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
