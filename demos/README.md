# Evidence Demos

This directory is reserved for focused demonstration programs that provide additional evidence for third-party libraries built and consumed with **Embarcadero C++Builder / BCC64X**.

A demo is not the BuildEngine application and it is not a product application. It exists to demonstrate a documented integration claim that is better shown through a small working program than through a minimal pass/fail smoke test.

Typical examples include rendering, networking/TLS, service interaction, or a representative public-library workflow.

The intended layout is:

```text
demos/<library>/<stable-demo-id>/
```

Demo source paths are normally version-independent. Version applicability and whether a demo is required evidence belong to the administration contracts, not to the directory name.

Third-party libraries remain under their own upstream licenses. Project-authored demo sources in this repository are licensed under the MIT License.
