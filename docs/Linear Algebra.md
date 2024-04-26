# Notes on important relationships in Linear Algebra

### Conventions

For this document, matrix dimensions will be denoted with $A \in \cnums^{c \times r}$, where columns are listed first. This goes counter to common maths conventions where rows are typically listed first and the letters $m$ and $n$ are used for rows and columns, respectively.

## Eigenvalues & Eigenvectors

Eigenvectors are "characteristic" vectors that, when transformed by their respective matrices do not change the line they're on. They may scale (including to start pointing in the opposite direction), but they still stay along the same line as before.

Note that the following statements are true in the general case over the complex numbers. There are some limitations to what kinds of matrices $ A $ will give only real-number solutions.

For any matrix $ A $, the equation $ A\vec{x} = \lambda \vec{x} $ is true for specific vectors $ \vec{x} $ and scalars $ \lambda $. These are the Eigenvectors and Eigenvalues, respectively. From this, you can see that eigenvectors have associated eigenvalues.

Given any matrix $A$, its associated eigenvector/eigenvalue pairs $\vec{x}, \lambda$, and the identity matrix $I$:
$$
A\vec{x} = \lambda I \vec{x} \\
A\vec{x} - \lambda I \vec{x} = 0 \\
(A-\lambda I)\vec{x} = 0
$$
This is true in the following cases:
$$
\vec{x} = 0 \\
\vec{x} \neq 0 \text{ and } \det{(A-\lambda I)} = 0
$$
We don't care about the first case because it's not useful. The second case, however, is very useful. We assert that $\vec{x} \neq 0$ and solve for the roots of the polynomial equation $\det{(A-\lambda I)} = 0$. The roots are the eigenvalues.

To then find the eigenvectors given each eigenvalue, you solve $(A-\lambda I)\vec{x} = 0$ for $\vec{x}$. Note that there are an infinite number of solutions that lie on the same line that crosses through the origin. Whichever you pick, it will be considered the same eigenvector direction, so any distinctions between them are not considered useful.

## Singular Value Decomposition

Any matrix transformation $ A $ can be described as rotating and stretching.

As such, any matrix $ A $ can be decomposed into rotation and scaling matrices. Note that rotation matrices are orthonormal and scaling matrices are diagonal. As such, to generalize to all matrices, we need 2 rotation matrices, and 1 scaling matrix, since the stretching directions in $ A $ are not always along the coordinate space axes, but by definition a diagonal matrix can only scale along the coordinate space axes. Therefore, our decomposition starts with a rotation matrix $ U $ to align the scaling directions to the coordinate space axes, then it's scaled by matrix $ \Sigma $, then rotated again by another matrix $ V^* $ to achieve the final orientation.

$$
A = U \Sigma V^*
$$

Note that $ V^* $ is the complex conjugate transpose of $ V $ where you take the transpose of $ V $ and negate any imaginary components. For $ V \in \R \quad V^* = V^\top $

This can be derived by taking our matrix $ A \in \cnums^{c \times r} $ to represent a transformation, defining $ \vec{v}_1, \vec{v}_2, ..., \vec{v}_c $ and $ \vec{u}_1, \vec{u}_2, ..., \vec{u}_c $ as $r$-dimensional orthonormal basis vectors (we call these our "principle axes"), and $ \sigma_1, \sigma_2, ..., \sigma_c $ as stretching factors (called our "singular values") where $A \vec{v}_j = \sigma_j \vec{u}_j$

From these definitions, we can construct the following matrices:
$$
U \in \cnums^{c \times r} =
\begin{bmatrix}
\\
\vec{u}_1 & \vec{u}_2 & \dots & \vec{u}_c \\
\\
\end{bmatrix} \\

V \in \cnums^{c \times c} =
\begin{bmatrix}
\\
\vec{v}_1 & \vec{v}_2 & \dots & \vec{v}_c \\
\\
\end{bmatrix} \\

\Sigma \in \R^{c \times c} =
\begin{bmatrix}
\sigma_1 & 0 & \dots & 0 \\
0 & \sigma_2 & \dots & 0 \\
\vdots & \vdots & \ddots & 0 \\
0 & 0 & 0 & \sigma_c
\end{bmatrix}
$$
Using these, our full relation can be represented as:
$$
A V = U \Sigma \\
A V V^{-1} = U \Sigma V^{-1} \\
A = U \Sigma V^{-1} \\
$$
And since $ \vec{v}_j $ are orthonormal, $ V $ is orthogonal, so $ V^{-1} = V^* $ and we arrive at the full decomposition $ A = U \Sigma V^* $

Note that it's convention to define $\sigma_1 \ge \sigma_2 \ge ... \ge \sigma_n \ge 0$

### Relation to Eigenstuffs

The SVD can be turned into an eigenvalue problem with the following steps:
$$
A = U \Sigma V^* \\
\Sigma^* = \Sigma
$$
$$
A^* A = (U \Sigma V^*)^*(U \Sigma V^*) \\
A^* A = V \Sigma U^*U \Sigma V^* \\
A^* A = V \Sigma^2 V^* \\
A^* A V = V \Sigma^2 V^* V \\
A^* A V = V \Sigma^2 \\
$$
$$
B \vec{x} = \lambda \vec{x} \\
B = A^* A \\
\vec{x}_j = V_j \\
\lambda = \Sigma^2 \\
\lambda_j = \sigma_j^2 \\
$$
$$
AA^* = (U \Sigma V^*)(U \Sigma V^*)^* \\
AA^* = U \Sigma V^*V \Sigma U^* \\
AA^* = U \Sigma^2 U^* \\
AA^* U = U \Sigma^2 U^* U \\
AA^* U = U \Sigma^2 \\
$$
$$
C \vec{y} = \lambda \vec{y} \\
C = AA^* \\
\vec{y}_j = U_j \\
$$
By finding the eigenvectors and eigenvalues of $A^*A$ and $AA^*$, you get the principle axes and the singular values squared. Note that by multiplying $A$ by its conjugate transpose you guarantee that you're always finding eigenvectors on a self-adjoint (Hermitian) matrix, which guarantees that the eigenvalues are real, positive, and distinct.